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
#include "server/record_log.h"

DEFINE_int32(port, 8020, "Port to listen on");
DEFINE_string(listen_address, "0.0.0.0", "Address to listen on. Use 0.0.0.0 to listen on any address");
DEFINE_int32(num_threads, 10, "Number of threads to create in the thread pool");
DEFINE_string(datastore, "/home/services/openinstrument", "Base directory for datastore files");
DEFINE_int32(store_max_ram, 200, "Maximum amount of datastore objects to cache in RAM");

namespace openinstrument {

using http::HttpServer;
using http::HttpRequest;
using http::HttpReply;

class DiskDatastore {
 public:
  typedef unordered_map<string, proto::ValueStream *> MapType;

  DiskDatastore(const string &basedir)
    : basedir_(basedir),
      record_log_(basedir_) {
    ReplayRecordLog();
  }

  ~DiskDatastore() {
    for (MapType::iterator i = live_data_.begin(); i != live_data_.end(); ++i) {
      delete i->second;
    }
    live_data_.clear();
  }

  void ListVariables(const string &prefix, vector<string> *vars) {
    for (MapType::iterator i = live_data_.begin(); i != live_data_.end(); ++i) {
      proto::ValueStream *stream = i->second;
      if (!stream->has_variable())
        continue;
      if (stream->variable().find(prefix) == 0)
        vars->push_back(stream->variable());
    }
  }

  bool CompareMessage(const proto::ValueStream &a, const proto::ValueStream &b) {
    return a.variable() == b.variable();
  }

  bool CompareMessage(const proto::Value &a, const proto::Value &b) {
    return a.timestamp() == b.timestamp() && a.value() == b.value();
  }

  void Record(const string &variable, Timestamp timestamp, double value) {
    proto::Value *val = RecordNoLog(variable, timestamp, value);
    if (val)
      record_log_.Add(variable, *val);
  }

  void Record(const string &variable, double value) {
    Record(variable, Timestamp::Now(), value);
  }

  void GetRange(const string &variable, const Timestamp &start, const Timestamp &end, proto::ValueStream *outstream) {
    proto::ValueStream *instream = GetVariable(variable);
    if (!instream || !instream->value_size())
      return;
    outstream->set_variable(instream->variable());
    for (int i = 0; i < instream->value_size(); i++) {
      const proto::Value &value = instream->value(i);
      if (value.timestamp() >= static_cast<uint64_t>(start.ms()) &&
          ((end.ms() && value.timestamp() < static_cast<uint64_t>(end.ms())) || !end.ms())) {
        proto::Value *val = outstream->add_value();
        val->set_timestamp(value.timestamp());
        val->set_value(value.value());
      }
    }
  }

  vector<Variable> FindVariables(const string &variable) {
    vector<Variable> vars;
    Variable search(variable);
    if (search.variable().empty())
      return vars;
    for (MapType::iterator i = live_data_.begin(); i != live_data_.end(); ++i) {
      Variable thisvar(i->first);
      if (thisvar.variable() != search.variable())
        continue;
      bool found = true;
      for (Variable::MapType::const_iterator l = search.labels().begin(); l != search.labels().end(); ++l) {
        if (l->second == "*") {
          if (!thisvar.HasLabel(l->first)) {
            found = false;
            break;
          }
        } else if (thisvar.GetLabel(l->first) != l->second) {
          found = false;
          break;
        }
      }
      if (found)
        vars.push_back(thisvar.ToString());
    }
    return vars;
  }

 private:
  proto::ValueStream *GetOrCreateVariable(const string &variable) {
    proto::ValueStream *stream = GetVariable(variable);
    if (stream)
      return stream;
    return CreateVariable(variable);
  }

  proto::ValueStream *GetVariable(const string &variable) {
    MapType::iterator it = live_data_.find(variable);
    if (it == live_data_.end())
      return NULL;
    return it->second;
  }

  proto::ValueStream *CreateVariable(const string &variable) {
    proto::ValueStream *stream = new proto::ValueStream();
    stream->set_variable(variable);
    live_data_[variable] = stream;
    return stream;
  }

  proto::Value *RecordNoLog(const string &variable, Timestamp timestamp, double value) {
    proto::ValueStream *stream = GetOrCreateVariable(variable);
    proto::Value *val = stream->add_value();
    val->set_timestamp(timestamp.ms());
    val->set_value(value);
    return val;
  }

  void ReplayRecordLog() {
    try {
      VLOG(1) << "Replaying record log";
      proto::ValueStream stream;
      uint64_t num_points = 0, num_streams = 0;
      while (record_log_.ReplayLog(&stream)) {
        for (int i = 0; i < stream.value_size(); i++) {
          RecordNoLog(stream.variable(), stream.value(i).timestamp(), stream.value(i).value());
          num_points++;
        }
        num_streams++;
      }
      LOG(INFO) << "Replayed record log, got " << num_points << " points" << " in " << num_streams << " streams";
    } catch (exception &e) {
      LOG(WARNING) << "Couldn't replay record log: " << e.what();
    }
  }

  string basedir_;
  MapType live_data_;
  RecordLog record_log_;
};

class DataStoreServer {
 public:
  DataStoreServer(const string &addr, uint16_t port, int num_threads)
    : datastore(FLAGS_datastore),
      server_(HttpServer::NewServer(addr, port, num_threads)),
      static_dir_("/static", "static", server_.get()),
      favicon_file_("/favicon.ico", "static/favicon.ico", server_.get()),
      add_request_timer_("/openinstrument/store/add-requests"),
      list_request_timer_("/openinstrument/store/list-requests"),
      get_request_timer_("/openinstrument/store/get-requests") {
    //datastore.connect("openinstrument", "localhost", "openinstrument", "openinstrument");
    server_->request_handler()->AddPath("/add$", &DataStoreServer::handle_add, this);
    server_->request_handler()->AddPath("/list$", &DataStoreServer::handle_list, this);
    server_->request_handler()->AddPath("/get$", &DataStoreServer::handle_get, this);
    server_->AddExportHandler();
  }

  bool handle_get(const HttpRequest &request, HttpReply *reply) {
    proto::GetRequest req;
    if (!UnserializeProtobuf(request.body(), &req))
      throw runtime_error("Invalid request");
    if (req.variable().empty())
      throw runtime_error("No variable specified");

    proto::GetResponse response;
    // Loop through all variables that match the requested variable.
    vector<Variable> vars(datastore.FindVariables(req.variable()));
    VLOG(1) << "Found " << vars.size() << " variables matching " << req.variable();
    vector<proto::ValueStream> streams;

    try {
      // Retrieve all the variable streams requested, mutating them on the way
      BOOST_FOREACH(Variable &var, vars) {
        Timestamp start(req.min_timestamp() ? req.min_timestamp() : 0);
        Timestamp end(req.has_max_timestamp() ? req.has_max_timestamp() : Timestamp::Now());

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
      }

      // Perform any requested aggregation
      if (req.aggregation_size()) {
        for (int agg_i = 0; agg_i < req.aggregation_size(); agg_i++) {
          const proto::StreamAggregation &agg = req.aggregation(agg_i);
          uint64_t sample_interval = agg.sample_interval();
          if (!sample_interval)
            sample_interval = 30000;

          Variable var;
          if (streams.size())
            var.SetVariable(Variable(streams[0].variable()).variable());

          if (!agg.label_size()) {
            LOG(INFO) << "Throw away all labels";
            // Aggregate by variable only, throw away all labels
            proto::ValueStream output;
            if (agg.type() == proto::StreamAggregation::AVERAGE) {
              ValueStreamAverage(streams, sample_interval, &output);
            } else if (agg.type() == proto::StreamAggregation::SUM) {
              ValueStreamSum(streams, sample_interval, &output);
            } else if (agg.type() == proto::StreamAggregation::MIN) {
              ValueStreamMin(streams, sample_interval, &output);
            } else if (agg.type() == proto::StreamAggregation::MAX) {
              ValueStreamMax(streams, sample_interval, &output);
            } else if (agg.type() == proto::StreamAggregation::MEDIAN) {
              ValueStreamMedian(streams, sample_interval, &output);
            }
            output.set_variable(var.ToString());
            response.add_stream()->CopyFrom(output);
          } else {
            for (int i = 0; i < agg.label_size(); i++) {
              string label = agg.label(i);
              set<string> distinct_values;

              BOOST_FOREACH(proto::ValueStream &stream, streams) {
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
                BOOST_FOREACH(proto::ValueStream &stream, streams) {
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
      response.set_message(e.what());
    }

    reply->set_status(HttpReply::OK);
    reply->SetContentType("application/base64");
    if (!SerializeProtobuf(response, reply->mutable_body())) {
      reply->set_status(HttpReply::INTERNAL_SERVER_ERROR);
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
    vector<string> vars;
    datastore.ListVariables(req.prefix(), &vars);
    response.set_success(true);
    for (vector<string>::const_iterator i = vars.begin(); i != vars.end(); ++i) {
      proto::ValueStream *stream = response.add_stream();
      stream->set_variable(*i);
    }
    reply->set_status(HttpReply::OK);
    reply->SetContentType("application/base64");
    if (!SerializeProtobuf(response, reply->mutable_body())) {
      reply->set_status(HttpReply::INTERNAL_SERVER_ERROR);
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
      try {
        response.set_success(true);
        string var = req.stream(streamid).variable();
        if (var.at(0) != '/' || var.size() < 2 || var.find_first_of("\n\t ") != string::npos)
          throw runtime_error(StringPrintf("Invalid variable name"));
        for (int valueid = 0; valueid < req.stream(streamid).value_size(); valueid++) {
          Timestamp ts(req.stream(streamid).value(valueid).timestamp());
          double value = req.stream(streamid).value(valueid).value();
          if (ts.seconds() > now.seconds() + 1.0)
            // Allow up to 1 second clock drift
            throw runtime_error(StringPrintf("Attempt to set value in the future (t=%0.3f, now=%0.3f)", ts.seconds(),
                                             now.seconds()));
          if (ts.seconds() < now.seconds() - 86400 * 365)
            LOG(WARNING) << "Adding very old data point for " << var;

          datastore.Record(var, ts, value);
        }
      } catch (exception &e) {
        LOG(WARNING) << e.what();
        response.set_success(false);
        response.set_message(e.what());
        break;
      }
    }

    reply->set_status(HttpReply::OK);
    reply->SetContentType("application/base64");
    if (!SerializeProtobuf(response, reply->mutable_body())) {
      reply->set_status(HttpReply::INTERNAL_SERVER_ERROR);
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
