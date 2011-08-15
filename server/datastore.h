/*
 *  -
 *
 * (c) 2010 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_DATASTORE_H_
#define _OPENINSTRUMENT_LIB_DATASTORE_H_

#include <algorithm>
#include <iostream>
#include <memory>
#include "lib/string.h"
#include "lib/timer.h"
#include "server/hostport.h"

using namespace std;

namespace openinstrument {

typedef unordered_map<const char*, string> LabelMap;


class DataValue {
 public:
  DataValue() : value(0), timestamp(0) {}

  DataValue(int64_t ts, int64_t val) {
    timestamp = ts;
    value = val;
  }

  bool operator<(const DataValue &val) const {
    return timestamp < val.timestamp;
  }

  int64_t value;
  int64_t timestamp;
};


struct DataValueIterators {
  vector<DataValue>::iterator iter;
  vector<DataValue>::iterator end;
};


class ValueStream {
 public:
  ValueStream() : source_(NULL), oldest_(0), newest_(0) {}

  inline const string &variable() const {
    return variable_;
  }

  inline void set_variable(const string &variable) {
    variable_ = variable;
  }

  inline const LabelMap &labels() const {
    return labels_;
  }

  inline void set_labels(const LabelMap &labels) {
    labels_ = labels;
  }

  inline const HostPort *source() const {
    return source_;
  }

  inline void set_source(HostPort *source) {
    source_ = source;
  }

  // Add a value at a given timestamp to the list of values for this stream.
  // Requires that the timestamp is greater than all the timestamps currently stored.
  inline bool add_value(const int64_t timestamp, const int64_t value) {
    // Don't allow old data to be added to the list. This is a cheap way of enforcing that the data is a sorted list,
    // without having the complexity of using a sorted list class.
    if (timestamp < newest_) {
      LOG(WARNING) << "Attempting to add a value that is older than the newest";
      return false;
    }
    VLOG(1) << "Recording value at timestamp " << timestamp << ": " << value;
    values_.push_back(DataValue(timestamp, value));

    if (timestamp < oldest_ || oldest_ == 0)
      oldest_ = timestamp;
    newest_ = timestamp;
    return true;
  }

  // Find the index of the first value that is at least as new as <timestamp>.
  // Returns a pair of vector<DataValue>::iterator objects, where the first iterator is the first data value that
  // matches, and the second is the end of the list.
  //
  // You can iterate over the return like this:
  //
  // DataValueIterators iter = stream->lower_bound(1234567890);
  // for (; iter.iter != iter.end; ++iter.iter) {
  //   timestamp = iter.iter->timestamp;
  //   value = iter.iter->value;
  // }
  DataValueIterators lower_bound(int64_t timestamp) {
    DataValueIterators p;
    p.iter = std::lower_bound(values_.begin(), values_.end(), DataValue(timestamp, 0));
    p.end = values_.end();
    return p;
  }

  // Fetch a range of values between min_timestamp and max_timestamp inclusive.
  // Returns a pair of vector<DataValue>::iterator objects, where the first iterator is the first data value that
  // matches, and the second is the end of the list.
  //
  // You can iterate over the return like this:
  //
  // DataValueIterators iter = stream->fetch_range(1234567890);
  // for (; iter.first != iter.second; ++iter.first) {
  //   timestamp = iter.first->timestamp;
  //   value = iter.first->value;
  // }
  DataValueIterators fetch_range(int64_t min_timestamp, int64_t max_timestamp) {
    DataValueIterators p;
    p.iter = std::lower_bound(values_.begin(), values_.end(), DataValue(min_timestamp, 0));
    p.end = std::upper_bound(p.iter, values_.end(), DataValue(max_timestamp, 0));
    return p;
  }

  inline vector<DataValue> &values() {
    return values_;
  }

 private:
  string variable_;
  LabelMap labels_;
  HostPort *source_;
  vector<DataValue> values_;
  int64_t oldest_;
  int64_t newest_;
};


class DataStore {
 public:
  ~DataStore() {
    for (unordered_map<string, ValueStream *>::iterator i = streams_.begin(); i != streams_.end(); ++i) {
      delete i->second;
    }
  }

  ValueStream *get_stream(const HostPort *source, const string &variable) {
    string key = StringPrintf("%s:%s", source->as_string().c_str(), variable.c_str());
    if (streams_.find(key) == streams_.end())
      return NULL;
    return streams_[key];
  }

  bool record(HostPort *source, const string &variable, const LabelMap &labels, const int64_t timestamp,
              const int64_t value) {
    string key = StringPrintf("%s:%s", source->as_string().c_str(), variable.c_str());
    ValueStream *stream;
    // Find an existing stream for this variable with labels
    if (streams_.find(key) == streams_.end()) {
      // Create a new stream
      stream = new ValueStream;
      stream->set_variable(variable);
      stream->set_source(source);
      stream->set_labels(labels);
      streams_[key] = stream;
    } else {
      stream = streams_[key];
    }
    stream->add_value(timestamp, value);
    return true;
  }

 private:
  unordered_map<string, ValueStream *> streams_;
};

}  // namespace

#endif  //_OPENINSTRUMENT_LIB_DATASTORE_H_ 
