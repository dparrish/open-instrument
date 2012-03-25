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
#include <gflags/gflags.h>
#include <glog/logging.h>
#include "lib/atomic.h"
#include "lib/common.h"
#include "lib/counter.h"
#include "lib/http_server.h"
#include "lib/http_static_dir.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/retention_policy.h"
#include "lib/string.h"
#include "lib/threadpool.h"
#include "lib/timer.h"
#include "server/datastore.h"
#include "server/disk_datastore.h"
#include "server/record_log.h"
#include "server/store_config.h"

DECLARE_int32(v);
DEFINE_int32(port, 8020, "Port to listen on");
DEFINE_string(listen_address, "::", "Address to listen on. Use 0.0.0.0 or :: to listen on any address");
DEFINE_int32(num_http_threads, 10, "Number of threads to create in the Http Server thread pool");
DEFINE_int32(max_http_threads, 20, "Max of threads to create in the Http Server thread pool");
DEFINE_string(datastore, "/home/services/openinstrument", "Base directory for datastore files");
DEFINE_int32(store_max_ram, 200, "Maximum amount of datastore objects to cache in RAM");
DEFINE_string(config_file, "config.txt", "Configuration file to read on startup");

namespace openinstrument {

using http::HttpServer;
using http::HttpRequest;
using http::HttpReply;

class DataStoreServer : private noncopyable {
 public:
  DataStoreServer(const string &addr, uint16_t port)
    : datastore(FLAGS_datastore),
      thread_pool_policy_(FLAGS_num_http_threads, FLAGS_max_http_threads),
      thread_pool_("datastore_server", thread_pool_policy_),
      server_(addr, port, &thread_pool_),
      static_dir_("/static", "static", &server_),
      favicon_file_("/favicon.ico", "static/favicon.ico", &server_),
      store_config_(StringPrintf("%s/config.txt", FLAGS_datastore.c_str())),
      retention_policy_(&store_config_),
      add_request_timer_("/openinstrument/store/add-requests"),
      list_request_timer_("/openinstrument/store/list-requests"),
      get_request_timer_("/openinstrument/store/get-requests"),
      forwarded_streams_ratio_("/openinstrument/store/forwarded-streams"),
      retention_policy_drops_("/openinstrument/store/retention-policy/values-dropped") {
    store_config_.WaitForConfigLoad();
    if (!store_config_.server(MyAddress()))
      throw runtime_error(StringPrintf("Could not find local address %s in store config", MyAddress().c_str()));
    store_config_.SetServerState(MyAddress(), proto::StoreServer::STARTING);
    server_.request_handler()->AddPath("/add$", &DataStoreServer::HandleAdd, this);
    server_.request_handler()->AddPath("/list$", &DataStoreServer::HandleList, this);
    server_.request_handler()->AddPath("/get$", &DataStoreServer::HandleGet, this);
    server_.request_handler()->AddPath("/get_config$", &DataStoreServer::HandleGetConfig, this);
    server_.request_handler()->AddPath("/health$", &DataStoreServer::HandleHealth, this);
    server_.AddExportHandler();
    // Export stats every minute
    VariableExporter::GetGlobalExporter()->SetExportLabel("job", "datastore");
    VariableExporter::GetGlobalExporter()->SetExportLabel("hostname", Socket::Hostname());
    VariableExporter::GetGlobalExporter()->StartExportThread(MyAddress(), 60);
    store_config_.SetServerState(MyAddress(), proto::StoreServer::RUNNING);
  }

  bool HandleGetConfig(const HttpRequest &request, HttpReply *reply) {
    reply->SetStatus(HttpReply::OK);
    reply->SetContentType("application/base64");
    if (!SerializeProtobuf(store_config_.config(), reply->mutable_body())) {
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
      BOOST_FOREACH(const Variable &var, vars) {
        if (++varcounter > req.max_variables())
          break;
        // Default to retrieving the last day
        Timestamp start(req.has_min_timestamp() ? req.min_timestamp() : Timestamp::Now() - (86400 * 1000));
        Timestamp end(req.has_max_timestamp() ? req.max_timestamp() : Timestamp::Now());

        if (req.mutation_size()) {
          proto::ValueStream initial_stream;
          datastore.GetRange(var, start, end, &initial_stream);
          for (int i = 0; i < req.mutation_size(); i++) {
            streams.push_back(proto::ValueStream());
            ApplyMutation(req.mutation(i), initial_stream, &streams.back());
          }
        } else {
          streams.push_back(proto::ValueStream());
          datastore.GetRange(var, start, end, &streams.back());
        }
        unique_vars.insert(var.variable());
      }

      // Perform any requested aggregation
      if (req.aggregation_size()) {
        BOOST_FOREACH(string varname, unique_vars) {
          // Get a list of all streams with the same variable name
          vector<proto::ValueStream> aggstreams;
          BOOST_FOREACH(proto::ValueStream &stream, streams) {
            if (Variable(stream.variable()).variable() == varname) {
              aggstreams.push_back(stream);
            }
          }
          for (int agg_i = 0; agg_i < req.aggregation_size(); agg_i++) {
            const proto::StreamAggregation &agg = req.aggregation(agg_i);
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
              for (int i = 0; i < agg.label_size(); i++) {
                string label = agg.label(i);
                set<string> distinct_values;

                BOOST_FOREACH(proto::ValueStream &stream, aggstreams) {
                  Variable tmpvar(stream.variable());
                  if (tmpvar.HasLabel(label))
                    distinct_values.insert(tmpvar.GetLabel(label));
                }
                VLOG(2) << "Distinct values for " << label << ":";
                BOOST_FOREACH(string output_label, distinct_values) {
                  VLOG(2) << "  " << output_label;
                  vector<proto::ValueStream> tmpstreams;
                  unordered_map<string, int> other_label_counts;
                  unordered_map<string, string> other_label_values;
                  BOOST_FOREACH(proto::ValueStream &stream, aggstreams) {
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
        BOOST_FOREACH(proto::ValueStream &stream, streams) {
          response.add_stream()->CopyFrom(stream);
        }
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
      for (int i = 0; i < istream.value_size(); i++) {
        proto::ValueStream tmpstream = ts.AddPoint(istream.value(i).timestamp(), istream.value(i).double_value());
        for (int j = 0; j < tmpstream.value_size(); j++) {
          proto::Value *newvalue = ostream->add_value();
          newvalue->CopyFrom(tmpstream.value(j));
        }
      }
    } else if (mutation.sample_type() == proto::StreamMutation::RATE ||
               mutation.sample_type() == proto::StreamMutation::RATE_SIGNED) {
      double lastval = 0;
      uint64_t lastts = 0;
      for (int i = 0; i < istream.value_size(); i++) {
        const proto::Value &oldvalue = istream.value(i);
        if (i > 0) {
          double rate = (oldvalue.double_value() - lastval) / ((oldvalue.timestamp() - lastts) / 1000.0);
          if (rate >= 0 || mutation.sample_type() == proto::StreamMutation::RATE_SIGNED) {
            proto::Value *newvalue = ostream->add_value();
            newvalue->set_timestamp(oldvalue.timestamp());
            newvalue->set_double_value(rate);
          }
        }
        lastval = oldvalue.double_value();
        lastts = oldvalue.timestamp();
      }
    } else if (mutation.sample_type() == proto::StreamMutation::DELTA) {
      double lastval = 0;
      for (int i = 0; i < istream.value_size(); i++) {
        const proto::Value &oldvalue = istream.value(i);
        if (i > 0) {
          double delta = oldvalue.double_value() - lastval;
          if (delta >= 0) {
            proto::Value *newvalue = ostream->add_value();
            newvalue->set_timestamp(oldvalue.timestamp());
            newvalue->set_double_value(delta);
          }
        }
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
    set<Variable> vars = datastore.FindVariables(req.prefix().name());

    response.set_success(true);
    uint32_t varcounter = 0;
    BOOST_FOREACH(const Variable &var, vars) {
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
          throw runtime_error(StringPrintf("Invalid variable name"));
        }
        string node = store_config_.hash_ring().GetNode(var.ToString());
        if (node != MyAddress() && !req.forwarded()) {
          // This variable should be stored on another storage server, add it to the list of streams to forward
          proto::AddRequest &forwardreq = forward_requests[node];
          forwardreq.set_forwarded(true);
          proto::ValueStream *forwardstream = forwardreq.add_stream();
          forwardstream->CopyFrom(*stream);
          continue;
        }
        for (int valueid = 0; valueid < stream->value_size(); valueid++) {
          const proto::Value &value = stream->value(valueid);
          Timestamp ts(value.timestamp());
          if (retention_policy_.ShouldDrop(retention_policy_.GetPolicy(var, now.ms() - ts.ms()))) {
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

 private:
  const string &MyAddress() const {
    static string my_address;
    if (my_address.empty()) {
      vector<Socket::Address> local_addresses = Socket::LocalAddresses();
      BOOST_FOREACH(Socket::Address &address, local_addresses) {
        address.set_port(server_.address().port());
        for (int i = 0 ; i < store_config_.config().server_size(); i++) {
          const proto::StoreServer &server = store_config_.config().server(i);

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
  DefaultThreadPoolPolicy thread_pool_policy_;
  ThreadPool thread_pool_;
  HttpServer server_;
  http::HttpStaticDir static_dir_;
  http::HttpStaticFile favicon_file_;
  StoreConfig store_config_;
  RetentionPolicy retention_policy_;

  ExportedTimer add_request_timer_;
  ExportedTimer list_request_timer_;
  ExportedTimer get_request_timer_;
  ExportedRatio forwarded_streams_ratio_;
  ExportedInteger retention_policy_drops_;
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
