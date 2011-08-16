/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <string>
#include "lib/common.h"
#include "lib/openinstrument.pb.h"
#include "lib/store_client.h"
#include "lib/string.h"
#include "lib/timer.h"

using namespace openinstrument;
using namespace std;

DEFINE_string(duration, "12h", "Duration of data to request");
DEFINE_string(interval, "", "Interval between output samples. Provide an empty string to get the raw data.");
DEFINE_string(sum_by, "", "Sum the output values by <sum_by>");
DEFINE_string(mean_by, "", "Average the output values by <sum_by>");
DEFINE_string(max_by, "", "Max the output values by <sum_by>");
DEFINE_string(min_by, "", "Min the output values by <sum_by>");
DEFINE_string(median_by, "", "Min the output values by <sum_by>");

void usage(char *argv[]) {
  cerr << "\nUsage: \n"
       << "\t" << argv[0] << " <server:port> <prefix> [aggregate label]...\n\n"
       << "  <prefix> should be of the form \"/var/name/here\".\n"
       << "\n";
  exit(1);
}

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  if (argc < 3) {
    cerr << "Wrong number of arguments\n";
    usage(argv);
  }

  proto::GetRequest req;
  req.set_variable(argv[2]);
  req.set_min_timestamp(Timestamp().ms() - Duration::FromString(FLAGS_duration).ms());

  if (FLAGS_interval.size()) {
    Duration interval(FLAGS_interval);
    if (interval.ms()) {
      proto::StreamMutation *mutation = req.add_mutation();
      mutation->set_sample_type(proto::StreamMutation::AVERAGE);
      mutation->set_sample_frequency(interval.ms()); // 2 minute samples
    }
  }

  // Average by variable
  if (FLAGS_sum_by.size()) {
    proto::StreamAggregation *agg = req.add_aggregation();
    agg->set_type(proto::StreamAggregation::SUM);
    agg->add_label(FLAGS_sum_by);
  }
  if (FLAGS_mean_by.size()) {
    proto::StreamAggregation *agg = req.add_aggregation();
    agg->set_type(proto::StreamAggregation::AVERAGE);
    agg->add_label(FLAGS_mean_by);
  }
  if (FLAGS_min_by.size()) {
    proto::StreamAggregation *agg = req.add_aggregation();
    agg->set_type(proto::StreamAggregation::MIN);
    agg->add_label(FLAGS_min_by);
  }
  if (FLAGS_max_by.size()) {
    proto::StreamAggregation *agg = req.add_aggregation();
    agg->set_type(proto::StreamAggregation::MAX);
    agg->add_label(FLAGS_max_by);
  }
  if (FLAGS_median_by.size()) {
    proto::StreamAggregation *agg = req.add_aggregation();
    agg->set_type(proto::StreamAggregation::MEDIAN);
    agg->add_label(FLAGS_median_by);
  }

  StoreClient client(argv[1]);
  scoped_ptr<proto::GetResponse> response(client.Get(req));
  LOG(INFO) << "Number of returned streams: " << response->stream_size();
  for (int stream_i = 0; stream_i < response->stream_size(); stream_i++) {
    const proto::ValueStream &stream = response->stream(stream_i);
    for (int i = 0; i < stream.value_size(); i++) {
      const proto::Value &value = stream.value(i);
      cout << StringPrintf("%s\t%s\t%f\n", stream.variable().c_str(),
                           Timestamp(value.timestamp()).GmTime("%Y-%m-%d %H:%M:%S").c_str(), value.value());
    }
  }

  return 0;
}
