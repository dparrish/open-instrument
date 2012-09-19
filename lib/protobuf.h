/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_PROTOBUF_H_
#define _OPENINSTRUMENT_LIB_PROTOBUF_H_

#include <string>
#include <vector>
#include <google/protobuf/message.h>
#include "lib/common.h"
#include "lib/cord.h"
#include "lib/file.h"
#include "lib/openinstrument.pb.h"
#include "lib/string.h"

namespace openinstrument {

string ProtobufText(const google::protobuf::Message &proto);

// Serialize a protobuf message and base64 encode it. Write the result to <output>
bool SerializeProtobuf(const google::protobuf::Message &proto, Cord *output);

// Base64 decode and write the de-serialized result to <proto>
bool UnserializeProtobuf(const Cord &input, google::protobuf::Message *proto);

void ValueStreamAverage(const vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output);
void ValueStreamMin(const vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output);
void ValueStreamMax(const vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output);
void ValueStreamMedian(const vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output);
void ValueStreamSum(const vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output);

class ProtoStreamReader : private noncopyable {
 public:
  explicit ProtoStreamReader(const string &filename) : fh_(filename) {}
  bool Skip(int count = 1);
  bool Next(google::protobuf::Message *msg);
  File *fh() {
    return &fh_;
  }

 private:
  bool FindNextHeader();

  string buf_;
  MmapFile fh_;
};

class ProtoStreamWriter : private noncopyable {
 public:
  explicit ProtoStreamWriter(const string &filename) : fh_(filename, "a") {}
  bool Write(const google::protobuf::Message &msg);
  File *fh() {
    return &fh_;
  }

 private:
  string buf_;
  File fh_;
};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_PROTOBUF_H_
