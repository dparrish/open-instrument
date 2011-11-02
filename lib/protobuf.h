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
  explicit ProtoStreamReader(const string &filename) : fh_(filename, "r") {}
  bool Skip(int count = 1);
  bool Next(google::protobuf::Message *msg, bool continue_on_error = true);
  void Reset();

 private:
  bool FindNextHeader();

  string buf_;
  File fh_;
};

class ProtoStreamWriter : private noncopyable {
 public:
  explicit ProtoStreamWriter(const string &filename) : fh_(filename, "a") {}
  bool Write(const google::protobuf::Message &msg);

 private:
  string buf_;
  File fh_;
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
  Variable() {}

  Variable(const string &input) {
    FromString(input);
  }

  Variable(const proto::Variable &input) {
    CopyFrom(input);
  }

  void FromString(const string &input);
  const string ToString() const;

  inline bool operator<(const Variable &b) const {
    return ToString() < b.ToString();
  }

  inline const string &name() const {
    return variable_.name();
  }

  inline void SetVariable(const proto::Variable &variable) {
    variable_ = variable;
  }

  inline void RemoveLabel(const string &label) {
    proto::Variable newvar(variable_);
    newvar.clear_label();
    bool found = false;
    for (int i = 0; i < variable_.label_size(); i++) {
      if (variable_.label(i).name() == label)
        continue;
      proto::VariableLabel *lab = newvar.add_label();
      lab->CopyFrom(variable_.label(i));
      found = true;
    }
    if (found)
      variable_.CopyFrom(newvar);
  }

  inline void SetLabel(const string &label, const string &value) {
    for (int i = 0; i < variable_.label_size(); i++) {
      if (variable_.label(i).name() == label) {
        variable_.mutable_label(i)->set_value(value);
        return;
      }
    }
    proto::VariableLabel *l = variable_.add_label();
    l->set_name(label);
    l->set_value(value);
  }

  inline bool HasLabel(const string &label) const {
    for (int i = 0; i < variable_.label_size(); i++) {
      if (variable_.label(i).name() == label)
        return true;
    }
    return false;
  }

  inline void CopyFrom(const proto::Variable &var) {
    variable_.CopyFrom(var);
  }

  inline void CopyTo(proto::Variable *var) const {
    var->CopyFrom(variable_);
  }

  inline const string &GetLabel(const string &label) const {
    static string emptystring;
    for (int i = 0; i < variable_.label_size(); i++) {
      if (variable_.label(i).name() == label)
        return variable_.label(i).value();
    }
    return emptystring;
  }

  bool Matches(const Variable &search) const;
  inline bool Matches(const string &search) const {
    return Matches(Variable(search));
  }

  inline bool operator==(const Variable &search) const {
    return equals(search);
  }

  inline bool operator==(const string &search) const {
    return equals(Variable(search));
  }

  bool equals(const Variable &search) const;
  inline bool equals(const string &search) const {
    return equals(Variable(search));
  }

  inline const proto::Variable &proto() const {
    return variable_;
  }

  inline proto::Variable *mutable_proto() {
    return &variable_;
  }

  inline string ValueTypeString() const {
    switch (proto().value_type()) {
      case proto::Variable::GAUGE:
        return "GAUGE";
      case proto::Variable::COUNTER:
        return "COUNTER";
      default:
        return "UNKNOWN";
    }
  }

  vector<string> LabelNames() const {
    vector<string> labelnames;
    for (int i = 0; i < variable_.label_size(); i++) {
      labelnames.push_back(variable_.label(i).name());
    }
    return labelnames;
  }

 private:
  bool ShouldQuoteValue(const string &input) const;
  string QuoteValue(const string &input) const;
  bool IsValueChar(const char p) const;
  bool IsValueQuoteChar(const char p) const;

  proto::Variable variable_;
};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_PROTOBUF_H_
