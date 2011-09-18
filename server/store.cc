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
#include "lib/string.h"
#include "lib/timer.h"
#include "server/datastore.h"
#include "server/disk_datastore.h"
#include "server/record_log.h"

DECLARE_int32(v);
DEFINE_int32(port, 8020, "Port to listen on");
DEFINE_string(listen_address, "0.0.0.0", "Address to listen on. Use 0.0.0.0 to listen on any address");
DEFINE_int32(num_threads, 10, "Number of threads to create in the thread pool");
DEFINE_string(datastore, "/home/services/openinstrument", "Base directory for datastore files");
DEFINE_int32(store_max_ram, 200, "Maximum amount of datastore objects to cache in RAM");

namespace openinstrument {

using http::HttpServer;
using http::HttpRequest;
using http::HttpReply;

class DataStoreServer : private noncopyable {
 public:
  DataStoreServer(const string &addr, uint16_t port, int num_threads)
    : datastore(FLAGS_datastore),
      server_(HttpServer::NewServer(addr, port, num_threads)),
      static_dir_("/static", "static", server_.get()),
      favicon_file_("/favicon.ico", "static/favicon.ico", server_.get()),
      add_request_timer_("/openinstrument/store/add-requests"),
      list_request_timer_("/openinstrument/store/list-requests"),
      get_request_timer_("/openinstrument/store/get-requests") {
    server_->request_handler()->AddPath("/add$", &DataStoreServer::handle_add, this);
    server_->request_handler()->AddPath("/list$", &DataStoreServer::handle_list, this);
    server_->request_handler()->AddPath("/get$", &DataStoreServer::handle_get, this);
    server_->request_handler()->AddPath("/health$", &DataStoreServer::handle_health, this);
    server_->AddExportHandler();
    // Export stats every 5 minutes
    VariableExporter::GetGlobalExporter()->SetExportLabel("job", "datastore");
    VariableExporter::GetGlobalExporter()->SetExportLabel("hostname", Socket::Hostname());
    VariableExporter::GetGlobalExporter()->StartExportThread(StringPrintf("localhost:%lu", FLAGS_port), 300);
  }

  bool handle_health(const HttpRequest &request, HttpReply *reply) {
    // Default health check, just return "OK"
    reply->SetStatus(HttpReply::OK);
    reply->SetContentType("text/plain");
    reply->mutable_body()->CopyFrom("OK\n");
    return true;
  }

  bool handle_get(const HttpRequest &request, HttpReply *reply) {
    proto::GetRequest req;
    if (!UnserializeProtobuf(request.body(), &req)) {
      reply->SetStatus(HttpReply::BAD_REQUEST);
      reply->mutable_body()->clear();
      reply->mutable_body()->CopyFrom("Invalid Request\n");
      return true;
    }
    VLOG(2) << ProtobufText(req);
    if (req.variable().empty())
      throw runtime_error("No variable specified");

    proto::GetResponse response;
    // Loop through all variables that match the requested variable.
    set<Variable> vars(datastore.FindVariables(req.variable()));
    VLOG(1) << "Found " << vars.size() << " variables matching " << req.variable();
    vector<proto::ValueStream> streams;
    set<string> unique_vars;

    try {
      // Retrieve all the variable streams requested, mutating them on the way
      BOOST_FOREACH(const Variable &var, vars) {
        // Default to retrieving the last day
        Timestamp start(req.has_min_timestamp() ? req.min_timestamp() : Timestamp::Now() - (86400 * 1000));
        Timestamp end(req.has_max_timestamp() ? req.max_timestamp() : Timestamp::Now());

        if (req.mutation_size()) {
          proto::ValueStream initial_stream;
          datastore.GetRange(var.ToString(), start, end, &initial_stream);
          for (int i = 0; i < req.mutation_size(); i++) {
            streams.push_back(proto::ValueStream());
            ApplyMutation(req.mutation(i), initial_stream, &streams.back());
          }
        } else {
          streams.push_back(proto::ValueStream());
          datastore.GetRange(var.ToString(), start, end, &streams.back());
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
              var.SetVariable(Variable(aggstreams[0].variable()).variable());

            if (!agg.label_size()) {
              LOG(INFO) << "Throw away all labels";
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
              output.set_variable(var.ToString());
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

                  Variable outputvar(var.ToString());
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
                  output.set_variable(outputvar.ToString());
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
    ostream->set_variable(istream.variable());

    if (mutation.sample_type() == proto::StreamMutation::NONE) {
      ostream->CopyFrom(istream);
    } else if (mutation.sample_type() == proto::StreamMutation::AVERAGE) {
      UniformTimeSeries ts(mutation.sample_frequency());
      for (int i = 0; i < istream.value_size(); i++) {
        proto::ValueStream tmpstream = ts.AddPoint(istream.value(i).timestamp(), istream.value(i).value());
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
          double rate = (oldvalue.value() - lastval) / ((oldvalue.timestamp() - lastts) / 1000.0);
          if (rate >= 0 || mutation.sample_type() == proto::StreamMutation::RATE_SIGNED) {
            proto::Value *newvalue = ostream->add_value();
            newvalue->set_timestamp(oldvalue.timestamp());
            newvalue->set_value(rate);
          }
        }
        lastval = oldvalue.value();
        lastts = oldvalue.timestamp();
      }
    } else if (mutation.sample_type() == proto::StreamMutation::DELTA) {
      double lastval = 0;
      for (int i = 0; i < istream.value_size(); i++) {
        const proto::Value &oldvalue = istream.value(i);
        if (i > 0) {
          double delta = oldvalue.value() - lastval;
          if (delta >= 0) {
            proto::Value *newvalue = ostream->add_value();
            newvalue->set_timestamp(oldvalue.timestamp());
            newvalue->set_value(delta);
          }
        }
        lastval = oldvalue.value();
      }
    } else {
      LOG(ERROR) << "Unsupported mutation type, returning original data";
      ostream->CopyFrom(istream);
    }

    return true;
  }

  bool handle_list(const HttpRequest &request, HttpReply *reply) {
    ScopedExportTimer t(&list_request_timer_);

    proto::ListRequest req;
    if (!UnserializeProtobuf(request.body(), &req))
      throw runtime_error("Invalid request");
    if (req.prefix().empty())
      throw runtime_error("Empty prefix");

    proto::ListResponse response;
    set<Variable> vars = datastore.FindVariables(req.prefix());
    response.set_success(true);
    BOOST_FOREACH(const Variable &var, vars) {
      proto::ValueStream *stream = response.add_stream();
      stream->set_variable(var.ToString());
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

  bool handle_add(const HttpRequest &request, HttpReply *reply) {
    ScopedExportTimer t(&add_request_timer_);

    proto::AddRequest req;
    if (!UnserializeProtobuf(request.body(), &req))
      throw runtime_error("Invalid request");
    if (req.stream_size() <= 0)
      throw runtime_error("Empty proto::ValueStream");

    proto::AddResponse response;
    Timestamp now;
    for (int streamid = 0; streamid < req.stream_size(); streamid++) {
      proto::ValueStream *stream = req.mutable_stream(streamid);
      Variable var(stream->variable());
      if (var.GetLabel("hostname").empty()) {
        // Force "hostname" to be set on every value stream
        var.SetLabel("hostname", request.source.AddressToString());
      }
      // Canonicalize the variable name
      stream->set_variable(var.ToString());
      VLOG(2) << "Adding value for " << var.ToString();
      try {
        if (var.variable().at(0) != '/' ||
            var.variable().size() < 2 ||
            var.variable().find_first_of("\n\t ") != string::npos) {
          throw runtime_error(StringPrintf("Invalid variable name"));
        }
        for (int valueid = 0; valueid < stream->value_size(); valueid++) {
          Timestamp ts(stream->value(valueid).timestamp());
          double value = stream->value(valueid).value();
          if (ts.seconds() > now.seconds() + 1.0)
            // Allow up to 1 second clock drift
            throw runtime_error(StringPrintf("Attempt to set value in the future (t=%0.3f, now=%0.3f)", ts.seconds(),
                                             now.seconds()));
          if (ts.seconds() < now.seconds() - 86400 * 365)
            LOG(WARNING) << "Adding very old data point for " << var.ToString();

          datastore.Record(var.ToString(), ts, value);
        }
        response.set_success(true);
      } catch (exception &e) {
        LOG(WARNING) << e.what();
        response.set_success(false);
        response.set_errormessage(e.what());
        break;
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
  DiskDatastore datastore;
  scoped_ptr<HttpServer> server_;
  http::HttpStaticDir static_dir_;
  http::HttpStaticFile favicon_file_;

  ExportedIntegerSet stats_;
  ExportedTimer add_request_timer_;
  ExportedTimer list_request_timer_;
  ExportedTimer get_request_timer_;
};

}  // namespace openinstrument


int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  // Start the server
  openinstrument::DataStoreServer server(FLAGS_listen_address, FLAGS_port, FLAGS_num_threads);

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
