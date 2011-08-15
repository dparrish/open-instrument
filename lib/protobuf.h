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

#include <google/protobuf/message.h>
#include <string>
#include <vector>
#include "lib/common.h"
#include "lib/cord.h"
#include "lib/file.h"
#include "lib/openinstrument.pb.h"
#include "lib/string.h"

namespace openinstrument {

// Serialize a protobuf message and base64 encode it. Write the result to <output>
bool SerializeProtobuf(const google::protobuf::Message &proto, Cord *output);

// Base64 decode and write the de-serialized result to <proto>
bool UnserializeProtobuf(const Cord &input, google::protobuf::Message *proto);

void ValueStreamAverage(vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output);
void ValueStreamMin(vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output);
void ValueStreamMax(vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output);
void ValueStreamMedian(vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output);
void ValueStreamSum(vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output);

class ProtoStreamReader {
 public:
  ProtoStreamReader(const string &filename) : fh_(filename, "r") {}
  bool Skip(int count = 1);
  bool Next(google::protobuf::Message *msg);

 private:
  bool FindNextHeader();

  string buf_;
  LocalFile fh_;
};

class ProtoStreamWriter {
 public:
  ProtoStreamWriter(const string &filename) : fh_(filename, "a") {}
  bool Write(google::protobuf::Message &msg);

 private:
  string buf_;
  LocalFile fh_;
};

// Container for variable names
// Acceptable formats are:
//    <variable>
//    <variable>{label=value,label2=value2}
//    <variable>{label="quoted value"}
//
// Acceptable characters for variable are:
//    a-z A-Z 0-9  . _ - / * ,
// Acceptable characters for label are:
//    a-z A-Z 0-9  . _ - / *
// Acceptable characters for value are any UTF-8 character except NULL
class Variable {
 public:
  typedef std::map<string, string> MapType;

  Variable() {}

  Variable(const string &input) {
    FromString(input);
  }

  void FromString(const string &input);
  const string ToString() const;

  inline const string &variable() const {
    return variable_;
  }

  inline void SetVariable(const string &variable) {
    variable_ = variable;
  }

  inline void SetLabel(const string &label, const string &value) {
    labels_[label] = value;
  }

  inline bool HasLabel(const string &label) const {
    MapType::const_iterator it = labels_.find(label);
    return it != labels_.end();
  }

  const MapType &labels() const {
    return labels_;
  }

  inline const string &GetLabel(const string &label) const {
    static string emptystring;
    MapType::const_iterator it = labels_.find(label);
    if (it == labels_.end())
      return emptystring;
    return it->second;
  }

 private:
  bool ShouldQuoteValue(const string &input) const;
  string QuoteValue(const string &input) const;
  bool IsValueChar(const char p) const;
  bool IsValueQuoteChar(const char p) const;

  string variable_;
  MapType labels_;
};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_PROTOBUF_H_
