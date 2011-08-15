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

using namespace openinstrument;
using namespace std;

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
  req.set_min_timestamp(Timestamp().ms() - (3600 * 1000) * 12);
  proto::StreamMutation *mutation = req.add_mutation();
  mutation->set_sample_type(proto::StreamMutation::AVERAGE);
  mutation->set_sample_frequency(120 * 1000); // 2 minute samples

  // Average by variable
  if (argc > 3) {
    proto::StreamAggregation *agg = req.add_aggregation();
    agg->set_type(proto::StreamAggregation::SUM);
    for (int i = 3; i < argc; i++) {
      agg->add_label(argv[i]);
    }
  }
  //proto::StreamAggregation *agg = req.add_aggregation();
  //agg->set_type(proto::StreamAggregation::MIN);
  //proto::StreamAggregation *agg = req.add_aggregation();
  //agg->set_type(proto::StreamAggregation::MAX);
  //proto::StreamAggregation *agg = req.add_aggregation();
  //agg->set_type(proto::StreamAggregation::MEDIAN);

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
