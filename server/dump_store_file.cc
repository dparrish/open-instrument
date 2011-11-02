/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <iostream>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include "lib/common.h"
#include "lib/protobuf.h"
#include "server/record_log.h"

DEFINE_bool(dump_data, false, "Dump every variable and data point found");

using namespace openinstrument;
using namespace std;

int main(int argc, char *argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  bool recordlog = false;
  ProtoStreamReader reader(argv[1]);
  proto::StoreFileHeader header;
  if (!reader.Next(&header, false)) {
    cout << "Not a datastore header, assuming recordlog" << endl;
    recordlog = true;
    reader.Reset();
  }

  proto::ValueStream stream;
  unordered_map<string, uint64_t> vars_without_labels;
  unordered_map<string, uint64_t> vars_with_labels;
  bool old_data = false;
  while (reader.Next(&stream, true)) {
    Variable var(stream.variable());
    if (stream.has_old_variable()) {
      old_data = true;
      var = Variable(stream.old_variable());
      if (var.GetLabel("datatype") == "gauge") {
        var.mutable_proto()->set_value_type(proto::Variable::GAUGE);
        var.RemoveLabel("datatype");
      } else if (var.GetLabel("datatype") == "counter") {
        var.mutable_proto()->set_value_type(proto::Variable::COUNTER);
        var.RemoveLabel("datatype");
      }
    }

    if (FLAGS_dump_data) {
      for (int i = 0; i < stream.value_size(); i++) {
        cout << StringPrintf("%s %s %12llu %0.5f", var.ToString().c_str(),
                             Variable(stream.variable()).ValueTypeString().c_str(), stream.value(i).timestamp(),
                             stream.value(i).value()) << endl;
      }
    }

    vars_without_labels[var.name()]++;
    vars_with_labels[var.ToString()]++;
  }

  cout << "Distinct variables without labels: " << vars_without_labels.size() << endl;
  cout << "Distinct variables including labels: " << vars_with_labels.size() << endl;
  if (old_data)
    cout << "WARNING: file contains old data, consider upgrading with store_reindex " << argv[1] << endl;
}
