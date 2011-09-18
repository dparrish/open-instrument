/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <algorithm>
#include <string>
#include <vector>
#include <boost/crc.hpp>
#include <boost/tokenizer.hpp>
#include <google/protobuf/text_format.h>
#include "lib/base64.h"
#include "lib/common.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"

namespace openinstrument {

typedef uint16_t magic_type;
typedef int32_t size_type;
typedef uint16_t crc_type;

const magic_type protomagic = 0xDEAD;

string ProtobufText(const google::protobuf::Message &proto) {
  string output;
  google::protobuf::TextFormat::PrintToString(proto, &output);
  return output;
}

bool SerializeProtobuf(const google::protobuf::Message &proto, Cord *output) {
  string temp;
  if (!proto.SerializeToString(&temp))
    return false;
  output->CopyFrom(Base64Encode(reinterpret_cast<const unsigned char *>(temp.c_str()), temp.length()));
  return true;
}

bool UnserializeProtobuf(const Cord &input, google::protobuf::Message *proto) {
  string c;
  input.AppendTo(&c);
  return proto->ParseFromString(Base64Decode(c));
}

void ValueStreamCalculation(const vector<proto::ValueStream> &input, uint64_t sample_interval,
                            boost::function<double(vector<double>)> calcfunc, proto::ValueStream *output) {
  vector<int> iterators;
  for (size_t i = 0; i < input.size(); i++)
    iterators.push_back(0);

  output->set_variable(input[0].variable());
  vector<double> bucket;
  uint64_t ts = 0;

  while (true) {
    bool found = false;
    bool found_bracket = false;
    for (size_t i = 0; i < iterators.size(); i++) {
      if (iterators[i] >= input[i].value_size())
        continue;
      const proto::Value &val = input[i].value(iterators[i]);
      found = true;
      if (!ts)
        ts = val.timestamp();
      if (val.timestamp() >= ts - sample_interval && val.timestamp() <= ts + sample_interval) {
        found_bracket = true;
        bucket.push_back(val.value());
        iterators[i]++;
      }
    }
    if (!found)
      break;
    if (!found_bracket && bucket.size()) {
      proto::Value *value = output->add_value();
      value->set_timestamp(ts);
      value->set_value(calcfunc(bucket));
      bucket.clear();
      ts = 0;
    }
  }
}

double _DoAverage(vector<double> bucket) {
  double total = 0;
  BOOST_FOREACH(double i, bucket) {
    total += i;
  }
  return total / bucket.size();
}

void ValueStreamAverage(const vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output) {
  ValueStreamCalculation(input, sample_interval, &_DoAverage, output);
}

double _DoSum(vector<double> bucket) {
  double total = 0;
  BOOST_FOREACH(double i, bucket) {
    total += i;
  }
  return total;
}

void ValueStreamSum(const vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output) {
  ValueStreamCalculation(input, sample_interval, &_DoSum, output);
}

double _DoMax(vector<double> bucket) {
  double max = 0;
  for (size_t i = 0; i < bucket.size(); i++) {
    if (bucket[i] > max)
      max = bucket[i];
  }
  return max;
}

void ValueStreamMax(const vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output) {
  ValueStreamCalculation(input, sample_interval, &_DoMax, output);
}

double _DoMin(vector<double> bucket) {
  double min = bucket[0];
  for (size_t i = 0; i < bucket.size(); i++) {
    if (bucket[i] < min)
      min = bucket[i];
  }
  return min;
}

void ValueStreamMin(const vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output) {
  ValueStreamCalculation(input, sample_interval, &_DoMin, output);
}

double _DoMedian(vector<double> bucket) {
  std::sort(bucket.begin(), bucket.end());
  return bucket[bucket.size() / 2];
}

void ValueStreamMedian(const vector<proto::ValueStream> &input, uint64_t sample_interval, proto::ValueStream *output) {
  ValueStreamCalculation(input, sample_interval, &_DoMedian, output);
}

uint32_t Crc(const char *ptr, uint64_t size) {
  boost::crc_16_type crc;
  crc.process_bytes(ptr, size);
  return crc.checksum();
}

uint32_t Crc(const string &str) {
  return Crc(str.data(), str.size());
}


bool ProtoStreamReader::Skip(int count) {
  for (int i = 0; i < count; i++) {
    magic_type magic;
    if (fh_.Read(&magic, sizeof(magic)) != sizeof(magic))
      return false;
    if (magic != protomagic) {
      LOG(WARNING) << "Incorrect protobuf delimeter 1";
      FindNextHeader();
      continue;
    }
    size_type size;
    if (fh_.Read(&size, sizeof(size)) != sizeof(size))
      return false;
    if (size >= fh_.size()) {
      LOG(WARNING) << "Size in protobuf header is too large";
      FindNextHeader();
      continue;
    }
    fh_.SeekRel(size + sizeof(crc_type));
  }
  return true;
}

bool ProtoStreamReader::Next(google::protobuf::Message *msg) {
  while (true) {
    uint64_t startpos = fh_.Tell();
    magic_type magic;
    if (fh_.Read(&magic, sizeof(magic)) != sizeof(magic))
      return false;
    if (magic != protomagic) {
      // LOG(INFO) << "Corrupted protobuf magic at 0x" << std::hex << startpos;
      fh_.SeekAbs(startpos + 1);
      if (FindNextHeader())
        continue;
      return false;
    }
    size_type size;
    if (fh_.Read(&size, sizeof(size)) != sizeof(size))
      return false;
    if (size >= fh_.size()) {
      // LOG(INFO) << "Corrupted protobuf size at 0x" << std::hex << startpos;
      fh_.SeekAbs(startpos + 1);
      if (FindNextHeader())
        continue;
      return false;
    }
    buf_.resize(size);
    if (fh_.Read(const_cast<char *>(buf_.data()), size) != size) {
      // LOG(INFO) << "Short protobuf read at 0x" << std::hex << startpos;
      return false;
    }
    crc_type crc;
    if (fh_.Read(&crc, sizeof(crc)) != sizeof(crc))
      return false;
    if (crc != Crc(buf_)) {
      // LOG(INFO) << "Corrupted protobuf crc at 0x" << std::hex << startpos;
      fh_.SeekAbs(startpos + 1);
      if (FindNextHeader())
        continue;
      return false;
    }

    if (!msg->ParseFromString(buf_)) {
      // LOG(INFO) << "Corrupted protobuf at 0x" << std::hex << startpos;
      if (FindNextHeader())
        continue;
      return false;
    }
    return true;
  }
}

bool ProtoStreamReader::FindNextHeader() {
  magic_type magic;
  while (true) {
    uint64_t startpos = fh_.Tell();
    if (fh_.Read(&magic, sizeof(magic)) != sizeof(magic))
      return false;
    if (magic != protomagic) {
      fh_.SeekAbs(startpos + 1);
      continue;
    }
    size_type size;
    if (fh_.Read(&size, sizeof(size)) != sizeof(size))
      return false;
    if (size >= fh_.size() - 2) {
      fh_.SeekAbs(startpos + 1);
      continue;
    }
    fh_.SeekAbs(startpos);
    return true;
  }
  // EOF
  return false;
}

bool ProtoStreamWriter::Write(const google::protobuf::Message &msg) {
  if (!msg.SerializeToString(&buf_))
    return false;
  // Write proto magic token and size
  magic_type magic = protomagic;
  if (fh_.Write(magic, sizeof(magic)) != sizeof(magic)) {
    LOG(INFO) << "Short write for magic";
    return false;
  }
  size_type size = buf_.size();
  if (fh_.Write(size, sizeof(size)) != sizeof(size)) {
    LOG(INFO) << "Short write for size";
    return false;
  }
  if (fh_.Write(buf_) != static_cast<int32_t>(buf_.size())) {
    LOG(INFO) << "Short write for proto";
    return false;
  }
  crc_type crc = Crc(buf_);
  if (fh_.Write(crc, sizeof(crc)) != sizeof(crc)) {
    LOG(INFO) << "Short write for CRC";
    return false;
  }

  return true;
}

void Variable::FromString(const string &input) {
  size_t pos = input.find('{');
  if (pos == string::npos) {
    variable_ = input;
    labels_.empty();
    return;
  }
  variable_ = input.substr(0, pos);
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
    } else if (i->second[0] == '/' && i->second[i->second.size() - 1] == '/') {
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

}  // namespace openinstrument
