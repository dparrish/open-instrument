/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <vector>
#include "lib/common.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/counter.h"

namespace openinstrument {

proto::ValueStream UniformTimeSeries::AddPoint(uint64_t timestamp, double value) {
  proto::ValueStream output;
  points_input_++;
  if (points_input_ == 1) {
    // This is the first point added. The timestamp of this point will be the base for our series, and every
    // subsequent output point will be at base_timestamp_ + (interval_ * points_output_).
    base_timestamp_ = timestamp - (timestamp % interval_);
    points_output_++;
    last_input_timestamp_ = timestamp;
    last_input_value_ = value;
    return output;
  }
  while (true) {
    uint64_t next_output_timestamp = base_timestamp_ + (interval_ * points_output_);
    VLOG(2) << "Next output timestamp is " << next_output_timestamp;
    if (timestamp == next_output_timestamp) {
      // Ignore any intervening points, we've got the required point exactly.
      VLOG(2) << "  Got that exactly";
      points_output_++;
      proto::Value *newval = output.add_value();
      newval->set_timestamp(timestamp);
      newval->set_value(value);
      break;
    } else if (timestamp < next_output_timestamp) {
      VLOG(2) << "  Input timestamp " << timestamp << " is before next output timestamp " << next_output_timestamp;
      break;
    } else {
      while (timestamp > next_output_timestamp) {
        VLOG(2) << "  Input timestamp " << timestamp << " is after next output timestamp " << next_output_timestamp;
        // rate for the input period including this point times the duration into this input period
        double newvalue = (value - last_input_value_) / (timestamp - last_input_timestamp_) *
              (next_output_timestamp - last_input_timestamp_) + last_input_value_;
        VLOG(2) << "    Generated value for " << next_output_timestamp << " = " << newvalue;
        proto::Value *newval = output.add_value();
        newval->set_timestamp(next_output_timestamp);
        newval->set_value(newvalue);
        points_output_++;
        next_output_timestamp = base_timestamp_ + (interval_ * points_output_);
      }
      continue;
    }
  }
  last_input_timestamp_ = timestamp;
  last_input_value_ = value;

  return output;
}

}  // namespace openinstrument
