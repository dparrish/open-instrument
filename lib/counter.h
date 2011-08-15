/*
 * Creates a uniform time series out of a non-uniform series.
 * Interpolates between input values to create a straight line.
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef OPENINSTRUMENT_LIB_COUNTER_H_
#define OPENINSTRUMENT_LIB_COUNTER_H_

#include <vector>
#include "lib/common.h"
#include "lib/openinstrument.pb.h"

namespace openinstrument {

class UniformTimeSeries {
 public:
  // Args:
  //   interval - The duration between output points in ms
  UniformTimeSeries(uint64_t interval)
    : interval_(interval),
      base_timestamp_(0),
      points_input_(0),
      points_output_(0),
      last_input_timestamp_(0),
      last_input_value_(0) {
  }

  // Add a value to the time series.
  // Returns 0 or more output points since the last AddPoint call.
  proto::ValueStream AddPoint(uint64_t timestamp, double value);

 private:
  uint64_t interval_;
  uint64_t base_timestamp_;
  uint64_t points_input_;
  uint64_t points_output_;
  uint64_t last_input_timestamp_;
  double last_input_value_;
};

}  // namespace openinstrument

#endif  // OPENINSTRUMENT_LIB_COUNTER_H_
