/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <map>
#include <string>
#include <boost/tokenizer.hpp>
#include "lib/common.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/string.h"
#include "lib/variable.h"

namespace openinstrument {

void Variable::FromString(const string &input) {
  size_t pos = input.find('{');
  if (pos == string::npos) {
    variable_ = input;
    labels_.empty();
    return;
  }
  variable_ = input.substr(0, pos);
  type_ = proto::StreamVariable::UNKNOWN;
  string labelstring = input.substr(pos + 1, input.size() - pos - 2);  // Lose the trailing }

  typedef boost::tokenizer<boost::escaped_list_separator<char> > Tokenizer;
  Tokenizer tokens(labelstring);
  for (Tokenizer::iterator it = tokens.begin(); it != tokens.end(); ++it) {
    string lv(*it);
    size_t pos = lv.find("=");
    if (pos == string::npos) {
      LOG(WARNING) << "Invalid variable label \"" << lv << "\", does not contain '='";
      continue;
    }
    string label = lv.substr(0, pos);
    string value = lv.substr(pos + 1);
    SetLabel(label, value);
  }
}

const string Variable::ToString() const {
  string output = variable_;
  if (labels_.size()) {
    output += "{";
    for (MapType::const_iterator i = labels_.begin(); i != labels_.end(); ++i) {
      if (i->second.empty())
        continue;
      if (i != labels_.begin())
        output += ",";
      output += i->first;
      output += "=";
      if (ShouldQuoteValue(i->second)) {
        output += "\"";
        output += QuoteValue(i->second);
        output += "\"";
      } else {
        output += i->second;
      }
    }
    output += "}";
  }
  return output;
}

bool Variable::IsValueChar(const char p) const {
  if ((p >= 'a' && p <= 'z') || (p >= 'A' && p <= 'Z') || (p >= '0' && p <= '9'))
    return true;
  if (p == '_' || p == '-' || p == '.' || p == ' ' || p == '*' || p == '/')
    return true;
  return false;
}

bool Variable::IsValueQuoteChar(const char p) const {
  return (p == ',' || p == '\"');
}

bool Variable::ShouldQuoteValue(const string &input) const {
  for (const char *p = input.data(); p < input.data() + input.size(); p++) {
    if (!IsValueChar(*p))
      return true;
  }
  return false;
}

string Variable::QuoteValue(const string &input) const {
  string output;
  output.reserve(input.size());
  for (const char *p = input.data(); p < input.data() + input.size(); p++) {
    if (IsValueChar(*p)) {
      output += *p;
    } else if (IsValueQuoteChar(*p)) {
      output += '\\';
      output += *p;
    } else {
      output += *p;
    }
  }
  return output;
}

bool Variable::Matches(const Variable &search) const {
  if (search.variable_[search.variable_.size() - 1] == '*') {
    // Compare up to the trailing *
    if (variable_.substr(0, search.variable_.size() - 1) != search.variable_.substr(0, search.variable_.size() - 1))
      return false;
  } else {
    if (search.variable_ != variable_)
      return false;
  }
  for (MapType::const_iterator i = search.labels_.begin(); i != search.labels_.end(); ++i) {
    if (i->second == "*") {
      if (!HasLabel(i->first))
        return false;
    } else if (i->second[0] == '/' && i->second[i->second.size() - 1] == '/' && i->second.size() > 2) {
      // It's a regex match
      boost::regex regex(i->second.substr(1, i->second.size() - 2));
      if (!boost::regex_match(GetLabel(i->first), regex))
        return false;
    } else if (i->second != GetLabel(i->first)) {
      // Straight string match
      return false;
    }
  }
  return true;
}

bool Variable::equals(const Variable &search) const {
  if (search.variable_ != variable_)
    return false;
  if (search.labels_.size() != labels_.size())
    return false;
  for (MapType::const_iterator i = search.labels_.begin(); i != search.labels_.end(); ++i) {
    if (GetLabel(i->first) != i->second)
      return false;
  }
  for (MapType::const_iterator i = labels_.begin(); i != labels_.end(); ++i) {
    if (search.GetLabel(i->first) != i->second)
      return false;
  }
  return true;
}

void Variable::FromProtobuf(const proto::StreamVariable &protobuf) {
  variable_ = protobuf.name();
  if (protobuf.has_type())
    type_ = protobuf.type();
  labels_.clear();
  for (int i = 0; i < protobuf.label_size(); i++) {
    const proto::Label &label = protobuf.label(i);
    labels_[label.label()] = label.value();
  }
}

void Variable::ToProtobuf(proto::StreamVariable *protobuf) const {
  protobuf->set_name(variable_);
  protobuf->clear_label();
  if (type_ != proto::StreamVariable::UNKNOWN)
    protobuf->set_type(type_);
  for (MapType::const_iterator it = labels_.begin(); it != labels_.end(); ++it) {
    proto::Label *label = protobuf->add_label();
    label->set_label(it->first);
    label->set_value(it->second);
  }
}

}  // namespace openinstrument
