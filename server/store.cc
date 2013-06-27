/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <algorithm>
#include <numeric>
#include <set>
#include <string>
#include <typeinfo>
#include <vector>
#include <ctemplate/template.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include "lib/atomic.h"
#include "lib/common.h"
#include "lib/counter.h"
#include "lib/http_server.h"
#include "lib/http_static_dir.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/retention_policy_manager.h"
#include "lib/store_config.h"
#include "lib/string.h"
#include "lib/threadpool.h"
#include "lib/timer.h"
#include "server/disk_datastore.h"
#include "server/indexed_store_file.h"
#include "server/record_log.h"
#include "server/store_file_manager.h"

DECLARE_string(config_file);
DEFINE_int32(port, 8020, "Port to listen on");
DEFINE_string(listen_address, "::", "Address to listen on. Use 0.0.0.0 or :: to listen on any address");
DEFINE_int32(num_http_threads, 10, "Number of threads to create in the Http Server thread pool");
DEFINE_int32(max_http_threads, 20, "Max of threads to create in the Http Server thread pool");
DEFINE_string(datastore, "/home/services/openinstrument", "Base directory for datastore files");
DEFINE_int32(store_max_ram, 200, "Maximum amount of datastore objects to cache in RAM");

namespace openinstrument {

using http::HttpServer;
using http::HttpRequest;
using http::HttpReply;

class DataStoreServer : private noncopyable {
 public:
  DataStoreServer(const string &addr, uint16_t port)
    : datastore(FLAGS_datastore),
      retention_policy_manager_(),
      store_file_manager_(retention_policy_manager_),
      thread_pool_policy_(FLAGS_num_http_threads, FLAGS_max_http_threads),
      thread_pool_("datastore_server", thread_pool_policy_),
      server_(addr, port, &thread_pool_),
      static_dir_("/static", "static", &server_),
      favicon_file_("/favicon.ico", "static/favicon.ico", &server_),
      add_request_timer_("/openinstrument/store/add-requests"),
      list_request_timer_("/openinstrument/store/list-requests"),
      get_request_timer_("/openinstrument/store/get-requests"),
      forwarded_streams_ratio_("/openinstrument/store/forwarded-streams"),
      retention_policy_drops_("/openinstrument/store/retention-policy/values-dropped") {
    run_timer_.Start();
    StoreConfig &config = StoreConfig::get_manager();
    config.SetConfigFilename(StringPrintf("%s/%s", FLAGS_datastore.c_str(), FLAGS_config_file.c_str()));
    if (!config.server(MyAddress()))
      throw runtime_error(StringPrintf("Could not find local address %s in store config", MyAddress().c_str()));
    config.SetServerState(MyAddress(), proto::StoreServer::STARTING);
    server_.request_handler()->AddPath("/add$", &DataStoreServer::HandleAdd, this);
    server_.request_handler()->AddPath("/list$", &DataStoreServer::HandleList, this);
    server_.request_handler()->AddPath("/get$", &DataStoreServer::HandleGet, this);
    server_.request_handler()->AddPath("/get_config$", &DataStoreServer::HandleGetConfig, this);
    server_.request_handler()->AddPath("/health$", &DataStoreServer::HandleHealth, this);
    server_.request_handler()->AddPath("/status$", &DataStoreServer::HandleStatus, this);
    server_.AddExportHandler();
    // Export stats every minute
    VariableExporter::GetGlobalExporter()->SetExportLabel("job", "datastore");
    VariableExporter::GetGlobalExporter()->SetExportLabel("hostname", Socket::Hostname());
    VariableExporter::GetGlobalExporter()->StartExportThread(MyAddress(), 300);
    config.SetServerState(MyAddress(), proto::StoreServer::RUNNING);
  }

  bool HandleGetConfig(const HttpRequest &request, HttpReply *reply) {
    reply->SetStatus(HttpReply::OK);
    reply->SetContentType("application/base64");
    if (!SerializeProtobuf(StoreConfig::get(), reply->mutable_body())) {
      reply->SetStatus(HttpReply::INTERNAL_SERVER_ERROR);
      reply->mutable_body()->clear();
      reply->mutable_body()->CopyFrom("Error serializing protobuf\n");
    }
    return true;
  }

  bool HandleHealth(const HttpRequest &request, HttpReply *reply) {
    // Default health check, just return "OK"
    reply->SetStatus(HttpReply::OK);
    reply->SetContentType("text/plain");
    reply->mutable_body()->CopyFrom("OK\n");
    return true;
  }

  bool HandleGet(const HttpRequest &request, HttpReply *reply) {
    ScopedExportTimer t(&get_request_timer_);

    proto::GetRequest req;
    if (!UnserializeProtobuf(request.body(), &req)) {
      reply->SetStatus(HttpReply::BAD_REQUEST);
      reply->mutable_body()->clear();
      reply->mutable_body()->CopyFrom("Invalid Request\n");
      return true;
    }
    VLOG(2) << ProtobufText(req);
    if (req.variable().name().empty()) {
      reply->SetStatus(HttpReply::BAD_REQUEST);
      reply->mutable_body()->clear();
      reply->mutable_body()->CopyFrom("No variable specified\n");
      return true;
    }

    proto::GetResponse response;
    // Loop through all variables that match the requested variable.
    set<Variable> vars(datastore.FindVariables(Variable(req.variable())));
    VLOG(1) << "Found " << vars.size() << " variables matching " << req.variable().name();
    vector<proto::ValueStream> streams;
    set<string> unique_vars;

    try {
      uint32_t varcounter = 0;
      // Retrieve all the variable streams requested, mutating them on the way
      for (auto &var : vars) {
        if (++varcounter > req.max_variables())
          break;
        // Default to retrieving the last day
        Timestamp start(req.has_min_timestamp() ? req.min_timestamp() : Timestamp::Now() - (86400 * 1000));
        Timestamp end(req.has_max_timestamp() ? req.max_timestamp() : Timestamp::Now());

        if (req.mutation_size()) {
          proto::ValueStream initial_stream;
          datastore.GetRange(var, start, end, &initial_stream);
          for (auto &mutation : req.mutation()) {
            streams.push_back(proto::ValueStream());
            ApplyMutation(mutation, initial_stream, &streams.back());
          }
        } else {
          streams.push_back(proto::ValueStream());
          datastore.GetRange(var, start, end, &streams.back());
        }
        unique_vars.insert(var.variable());
      }

      // Perform any requested aggregation
      if (req.aggregation_size()) {
        for (auto &varname : unique_vars) {
          // Get a list of all streams with the same variable name
          vector<proto::ValueStream> aggstreams;
          for (auto &stream : streams) {
            if (Variable(stream.variable()).variable() == varname) {
              aggstreams.push_back(stream);
            }
          }
          for (auto &agg : req.aggregation()) {
            uint64_t sample_interval = agg.sample_interval();
            if (!sample_interval)
              sample_interval = 30000;

            Variable var;
            if (aggstreams.size())
              var.set_variable(Variable(aggstreams[0].variable()).variable());

            if (!agg.label_size()) {
              // Aggregate by variable only, throw away all labels
              proto::ValueStream output;
              if (agg.type() == proto::StreamAggregation::AVERAGE) {
                ValueStreamAverage(aggstreams, sample_interval, &output);
              } else if (agg.type() == proto::StreamAggregation::SUM) {
                ValueStreamSum(aggstreams, sample_interval, &output);
              } else if (agg.type() == proto::StreamAggregation::MIN) {
                ValueStreamMin(aggstreams, sample_interval, &output);
              } else if (agg.type() == proto::StreamAggregation::MAX) {
                ValueStreamMax(aggstreams, sample_interval, &output);
              } else if (agg.type() == proto::StreamAggregation::MEDIAN) {
                ValueStreamMedian(aggstreams, sample_interval, &output);
              }
              var.ToValueStream(&output);
              response.add_stream()->CopyFrom(output);
            } else {
              for (const string &label : agg.label()) {
                set<string> distinct_values;

                for (auto &stream : aggstreams) {
                  Variable tmpvar(stream.variable());
                  if (tmpvar.HasLabel(label))
                    distinct_values.insert(tmpvar.GetLabel(label));
                }
                VLOG(2) << "Distinct values for " << label << ":";
                for (const string &output_label : distinct_values) {
                  VLOG(2) << "  " << output_label;
                  vector<proto::ValueStream> tmpstreams;
                  unordered_map<string, int> other_label_counts;
                  unordered_map<string, string> other_label_values;
                  for (auto &stream : aggstreams) {
                    Variable tmpvar(stream.variable());
                    if (tmpvar.GetLabel(label) == output_label) {
                      tmpstreams.push_back(stream);
                      for (Variable::MapType::const_iterator it = tmpvar.labels().begin(); it != tmpvar.labels().end();
                           ++it) {
                        if (other_label_values[it->first] != it->second) {
                          ++other_label_counts[it->first];
                          other_label_values[it->first] = it->second;
                        }
                      }
                    }
                  }

                  Variable outputvar(var);
                  outputvar.SetLabel(label, output_label);

                  for (unordered_map<string, int>::iterator it = other_label_counts.begin();
                       it != other_label_counts.end(); ++it) {
                    if (it->second == 1) {
                      outputvar.SetLabel(it->first, other_label_values[it->first]);
                    }
                  }

                  if (!tmpstreams.size()) {
                    LOG(ERROR) << "Could not find any streams with " << label << " == " << output_label;
                    continue;
                  }
                  VLOG(1) << "Found " << tmpstreams.size() << " streams with " << label << " == " << output_label;

                  proto::ValueStream output;
                  if (agg.type() == proto::StreamAggregation::AVERAGE) {
                    ValueStreamAverage(tmpstreams, sample_interval, &output);
                  } else if (agg.type() == proto::StreamAggregation::SUM) {
                    ValueStreamSum(tmpstreams, sample_interval, &output);
                  } else if (agg.type() == proto::StreamAggregation::MIN) {
                    ValueStreamMin(tmpstreams, sample_interval, &output);
                  } else if (agg.type() == proto::StreamAggregation::MAX) {
                    ValueStreamMax(tmpstreams, sample_interval, &output);
                  } else if (agg.type() == proto::StreamAggregation::MEDIAN) {
                    ValueStreamMedian(tmpstreams, sample_interval, &output);
                  }
                  outputvar.ToValueStream(&output);
                  response.add_stream()->CopyFrom(output);
                }
              }
            }
          }
        }
      } else {
        // Write the final streams to the response object
        for (auto &stream : streams)
          response.add_stream()->CopyFrom(stream);
      }

      response.set_success(true);
    } catch (exception &e) {
      LOG(WARNING) << e.what();
      response.set_success(false);
      response.set_errormessage(e.what());
    }

    reply->SetStatus(HttpReply::OK);
    reply->SetContentType("application/base64");
    if (!SerializeProtobuf(response, reply->mutable_body())) {
      reply->SetStatus(HttpReply::INTERNAL_SERVER_ERROR);
      reply->mutable_body()->clear();
      reply->mutable_body()->CopyFrom("Error serializing protobuf\n");
    }
    return true;
  }

  bool ApplyMutation(const proto::StreamMutation &mutation, proto::ValueStream &istream, proto::ValueStream *ostream) {
    if ((mutation.sample_type() == proto::StreamMutation::AVERAGE ||
         mutation.sample_type() == proto::StreamMutation::MIN ||
         mutation.sample_type() == proto::StreamMutation::MAX) &&
        (!mutation.has_sample_frequency() || !mutation.max_gap_interpolate())) {
      LOG(WARNING) << "Invalid StreamMutation";
      throw runtime_error("Invalid StreamMutation");
    }
    if (!istream.value_size()) {
      LOG(WARNING) << "Empty StreamMutation";
      return true;
    }
    ostream->mutable_variable()->CopyFrom(istream.variable());

    if (mutation.sample_type() == proto::StreamMutation::NONE) {
      ostream->CopyFrom(istream);
    } else if (mutation.sample_type() == proto::StreamMutation::AVERAGE) {
      UniformTimeSeries ts(mutation.sample_frequency());
      for (auto &ivalue : istream.value()) {
        proto::ValueStream tmpstream = ts.AddPoint(ivalue.timestamp(), ivalue.double_value());
        for (auto &tmpvalue : tmpstream.value()) {
          proto::Value *newvalue = ostream->add_value();
          newvalue->CopyFrom(tmpvalue);
        }
      }
    } else if (mutation.sample_type() == proto::StreamMutation::RATE ||
               mutation.sample_type() == proto::StreamMutation::RATE_SIGNED) {
      double lastval = 0;
      uint64_t lastts = 0;
      bool first = true;
      for (auto &oldvalue : istream.value()) {
        if (!first) {
          double rate = (oldvalue.double_value() - lastval) / ((oldvalue.timestamp() - lastts) / 1000.0);
          if (rate >= 0 || mutation.sample_type() == proto::StreamMutation::RATE_SIGNED) {
            proto::Value *newvalue = ostream->add_value();
            newvalue->set_timestamp(oldvalue.timestamp());
            newvalue->set_double_value(rate);
          }
        }
        first = false;
        lastval = oldvalue.double_value();
        lastts = oldvalue.timestamp();
      }
    } else if (mutation.sample_type() == proto::StreamMutation::DELTA) {
      double lastval = 0;
      bool first = true;
      for (auto &oldvalue : istream.value()) {
        if (!first) {
          double delta = oldvalue.double_value() - lastval;
          if (delta >= 0) {
            proto::Value *newvalue = ostream->add_value();
            newvalue->set_timestamp(oldvalue.timestamp());
            newvalue->set_double_value(delta);
          }
        }
        first = false;
        lastval = oldvalue.double_value();
      }
    } else {
      LOG(ERROR) << "Unsupported mutation type, returning original data";
      ostream->CopyFrom(istream);
    }

    return true;
  }

  bool HandleList(const HttpRequest &request, HttpReply *reply) {
    ScopedExportTimer t(&list_request_timer_);

    proto::ListRequest req;
    if (!UnserializeProtobuf(request.body(), &req)) {
      reply->SetStatus(HttpReply::BAD_REQUEST);
      reply->mutable_body()->clear();
      reply->mutable_body()->CopyFrom("Invalid request\n");
      return true;
    }
    if (req.prefix().name().empty()) {
      reply->SetStatus(HttpReply::BAD_REQUEST);
      reply->mutable_body()->clear();
      reply->mutable_body()->CopyFrom("Empty pretix\n");
      return true;
    }
    proto::ListResponse response;
    set<Variable> vars = datastore.FindVariables(Variable(req.prefix()));

    response.set_success(true);
    uint32_t varcounter = 0;
    for (auto &var : vars) {
      if (++varcounter > req.max_variables())
        break;
      proto::ValueStream *stream = response.add_stream();
      var.ToValueStream(stream);
    }
    reply->SetStatus(HttpReply::OK);
    reply->SetContentType("application/base64");
    if (!SerializeProtobuf(response, reply->mutable_body())) {
      reply->SetStatus(HttpReply::INTERNAL_SERVER_ERROR);
      reply->mutable_body()->clear();
      reply->mutable_body()->CopyFrom("Error serializing protobuf\n");
    }
    return true;
  }

  bool HandleAdd(const HttpRequest &request, HttpReply *reply) {
    ScopedExportTimer t(&add_request_timer_);

    proto::AddRequest req;
    if (!UnserializeProtobuf(request.body(), &req)) {
      reply->SetStatus(HttpReply::BAD_REQUEST);
      reply->mutable_body()->clear();
      reply->mutable_body()->CopyFrom("Invalid request\n");
      return true;
    }
    if (req.stream_size() <= 0) {
      reply->SetStatus(HttpReply::BAD_REQUEST);
      reply->mutable_body()->clear();
      reply->mutable_body()->CopyFrom("Empty proto::ValueStream\n");
      return true;
    }

    proto::AddResponse response;
    response.set_success(true);
    unordered_map<string, proto::AddRequest> forward_requests;
    Timestamp now;
    for (int streamid = 0; streamid < req.stream_size(); streamid++) {
      proto::ValueStream *stream = req.mutable_stream(streamid);
      Variable var(stream->variable());
      if (var.GetLabel("hostname").empty()) {
        // Force "hostname" to be set on every value stream. This must be done before forwarding so that the source
        // address is correct.
        var.SetLabel("hostname", request.source.AddressToString());
      }
      // Canonicalize the variable name
      var.ToValueStream(stream);
      VLOG(1) << "Adding value for " << var.ToString();
      try {
        if (var.variable().at(0) != '/' ||
            var.variable().size() < 2 ||
            var.variable().find_first_of("\n\t ") != string::npos) {
          throw runtime_error(StringPrintf("Invalid variable name %s", var.ToString().c_str()));
        }
        string node = StoreConfig::get_manager().hash_ring().GetNode(var.ToString());
        if (node != MyAddress() && !req.forwarded()) {
          // This variable should be stored on another storage server, add it to the list of streams to forward
          proto::AddRequest &forwardreq = forward_requests[node];
          forwardreq.set_forwarded(true);
          proto::ValueStream *forwardstream = forwardreq.add_stream();
          forwardstream->CopyFrom(*stream);
          continue;
        }
        for (auto &value : stream->value()) {
          Timestamp ts(value.timestamp());
          if (retention_policy_manager_.ShouldDrop(retention_policy_manager_.GetPolicy(var, now.ms() - ts.ms()))) {
            // Retention policy says this variable should be dropped, so just ignore it
            ++retention_policy_drops_;
            continue;
          }
          if (ts.ms() > now.ms() + 1000)
            // Allow up to 1 second clock drift
            throw runtime_error(StringPrintf("Attempt to set value in the future (t=%0.3f, now=%0.3f)", ts.seconds(),
                                             now.seconds()));
          if (ts.seconds() < now.seconds() - 86400 * 365)
            LOG(WARNING) << "Adding very old data point for " << var.ToString();

          datastore.Record(var, ts, value);
          VLOG(1) << "Adding value for " << var.ToString() << " = " << std::fixed << value.double_value();
        }
      } catch (exception &e) {
        LOG(WARNING) << e.what();
        response.set_success(false);
        response.set_errormessage(e.what());
        break;
      }
    }

    // Forward on any streams that should go to other storage servers.
    for (unordered_map<string, proto::AddRequest>::iterator i = forward_requests.begin(); i != forward_requests.end();
         ++i) {
      try {
        StoreClient client(i->first);
        scoped_ptr<proto::AddResponse> response(client.Add(i->second));
        VLOG(3) << "Forwarded " << i->second.stream_size() << " streams to " << i->first << ", response is "
                << response->success();
        forwarded_streams_ratio_.success();
      } catch (exception e) {
        LOG(WARNING) << "Attempt to forward " << i->second.stream_size() << " streams to " << i->first
                     << " failed!";
        forwarded_streams_ratio_.failure();
      }
    }


    reply->SetStatus(HttpReply::OK);
    reply->SetContentType("application/base64");
    if (!SerializeProtobuf(response, reply->mutable_body())) {
      reply->SetStatus(HttpReply::INTERNAL_SERVER_ERROR);
      reply->mutable_body()->clear();
      reply->mutable_body()->CopyFrom("Error serializing protobuf\n");
    }
    return true;
  }

  bool HandleStatus(const HttpRequest &request, HttpReply *reply) {
    // Default health check, just return "OK"
    reply->SetStatus(HttpReply::OK);
    reply->SetContentType("text/html");
    ctemplate::TemplateDictionary dict("status");
    dict.SetValue("RUNTIME", Duration(run_timer_.ms()).ToString());

    dict.SetIntValue("STORE_TOTAL_FILES", store_file_manager_.available_files().size());
    dict.SetIntValue("STORE_OPEN_FILES", store_file_manager_.num_open_files());
    for (auto &filename : store_file_manager_.available_files()) {
      auto file = dict.AddSectionDictionary("STORE_FILES");
      file->SetValue("FILENAME", filename);
      try {
        auto &header = store_file_manager_.GetHeader(filename);
        file->SetValue("OPEN", "Yes");
        file->SetValue("FIRST", Timestamp(header.start_timestamp()).GmTime());
        file->SetValue("LAST", Timestamp(header.end_timestamp()).GmTime());
        file->SetIntValue("VARIABLES", header.index_size() ? header.index_size() : header.variable_size());
      } catch (const string &e) {
        file->SetValue("OPEN", "No");
      }
    }

    uint32_t total_variables = 0, total_values = 0, total_ram = 0;

    for (auto &variable : datastore.FindVariables(Variable("*"))) {
      const auto &stream = datastore.GetValueStream(variable);
      auto vdict = dict.AddSectionDictionary("LIVE_VARIABLE");
      uint32_t data_size = 0;
      vdict->SetValue("VARIABLE", variable.ToString());
      vdict->SetIntValue("COUNTER", stream.value_size());
      total_variables++;
      total_values += stream.value_size();
      if (stream.value_size()) {
        const auto &front = stream.value(0);
        const auto &back = stream.value(stream.value_size() - 1);
        vdict->SetValue("FIRST", Duration(Timestamp::Now() - front.timestamp()).ToString(false));
        vdict->SetValue("LAST", Duration(Timestamp::Now() - back.timestamp()).ToString(false));
        if (front.has_double_value())
          vdict->SetValue("LATEST", StringPrintf("%0.4f", front.double_value()));
        else if (front.has_string_value())
          vdict->SetValue("LATEST", "<em>&lt;STRING&gt;</em>");
        else
          vdict->SetValue("LATEST", "<em>none</em>");
      }
      for (const auto &value : stream.value()) {
        if (value.has_double_value())
          data_size += sizeof(value.double_value());
        if (value.has_string_value())
          data_size += value.string_value().size();
      }
      vdict->SetValue("DATA_SIZE", SiUnits(data_size, 1));
      total_ram += data_size;
    }

    dict.SetIntValue("TOTAL_RECORDLOG_VARIABLES", total_variables);
    dict.SetIntValue("TOTAL_RECORDLOG_VALUES", total_values);
    dict.SetValue("TOTAL_RAM", SiUnits(total_ram));

    string output;
    ctemplate::ExpandTemplate("server/status_template.html", ctemplate::DO_NOT_STRIP, &dict, &output);
    reply->mutable_body()->CopyFrom(output);
    return true;
  }

 private:
  const string &MyAddress() const {
    static string my_address;
    auto &config = StoreConfig::get();
    if (my_address.empty()) {
      vector<Socket::Address> local_addresses = Socket::LocalAddresses();
      for (auto &address : local_addresses) {
        address.set_port(server_.address().port());
        for (auto &server : config.server()) {
          if (server.address() == address.ToString()) {
            my_address = address.ToString();
            return my_address;
          }
        }
      }
      LOG(WARNING) << "Couldn't find store config address.";
    }

    return my_address;
  }

  DiskDatastore datastore;
  RetentionPolicyManager retention_policy_manager_;
  StoreFileManager store_file_manager_;
  DefaultThreadPoolPolicy thread_pool_policy_;
  ThreadPool thread_pool_;
  HttpServer server_;
  http::HttpStaticDir static_dir_;
  http::HttpStaticFile favicon_file_;

  ExportedTimer add_request_timer_;
  ExportedTimer list_request_timer_;
  ExportedTimer get_request_timer_;
  ExportedRatio forwarded_streams_ratio_;
  ExportedInteger retention_policy_drops_;
  Timer run_timer_;
};

}  // namespace openinstrument


int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  // Start the server
  openinstrument::DataStoreServer server(FLAGS_listen_address, FLAGS_port);

  // Wait for signal indicating time to shut down.
  sigset_t wait_mask;
  sigemptyset(&wait_mask);
  sigaddset(&wait_mask, SIGINT);
  sigaddset(&wait_mask, SIGQUIT);
  sigaddset(&wait_mask, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
  int sig = 0;
  sigwait(&wait_mask, &sig);

  return 0;
}
