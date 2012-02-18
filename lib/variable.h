/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_VARIABLE_H_
#define _OPENINSTRUMENT_LIB_VARIABLE_H_

#include <map>
#include <string>
#include "lib/common.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/string.h"

namespace openinstrument {

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

  Variable() : type_(proto::StreamVariable::UNKNOWN) {}

  Variable(const char *input) : type_(proto::StreamVariable::UNKNOWN) {
    FromString(string(input));
  }

  Variable(const string &input) : type_(proto::StreamVariable::UNKNOWN)  {
    FromString(input);
  }

  explicit Variable(const proto::StreamVariable &proto) : type_(proto::StreamVariable::UNKNOWN)  {
    FromProtobuf(proto);
  }

  void FromString(const string &input);
  const string ToString() const;

  void FromProtobuf(const proto::StreamVariable &protobuf);
  void FromValueStream(const proto::ValueStream &stream) {
    return FromProtobuf(stream.variable());
  }
  void ToProtobuf(proto::StreamVariable *protobuf) const;
  void ToValueStream(proto::ValueStream *stream) const {
    return ToProtobuf(stream->mutable_variable());
  }

  inline bool operator<(const Variable &b) const {
    return ToString() < b.ToString();
  }

  inline const string &variable() const {
    return variable_;
  }

  inline void set_variable(const string &variable) {
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

  inline Variable CopyAndAppend(const string &append) const {
    Variable newvar(*this);
    newvar.set_variable(variable() + append);
    return newvar;
  }

  inline const string &GetLabel(const string &label) const {
    static string emptystring;
    MapType::const_iterator it = labels_.find(label);
    if (it == labels_.end())
      return emptystring;
    return it->second;
  }

  proto::StreamVariable::ValueType type() const {
    return type_;
  }

  void set_type(proto::StreamVariable::ValueType type) {
    type_ = type;
  }

  bool is_gauge() const {
    return type_ == proto::StreamVariable::GAUGE;
  }

  void set_gauge() {
    type_ = proto::StreamVariable::GAUGE;
  }

  bool is_rate() const {
    return type_ == proto::StreamVariable::RATE;
  }

  void set_rate() {
    type_ = proto::StreamVariable::RATE;
  }

  bool Matches(const Variable &search) const;

  inline bool operator!=(const Variable &search) const {
    return !equals(search);
  }

  inline bool operator==(const Variable &search) const {
    return equals(search);
  }

  bool equals(const Variable &search) const;

  // Used to keep track of how much data is in RAM
  inline uint64_t RamSize() const {
    uint64_t size = sizeof(labels_) + variable_.capacity();
    for (MapType::const_iterator i = labels_.begin(); i != labels_.end(); ++i) {
      size += i->first.capacity() + i->second.capacity();
    }
    return size;
  }

 private:
  bool ShouldQuoteValue(const string &input) const;
  string QuoteValue(const string &input) const;
  bool IsValueChar(const char p) const;
  bool IsValueQuoteChar(const char p) const;

  string variable_;
  MapType labels_;
  proto::StreamVariable::ValueType type_;
};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_PROTOBUF_H_
