package mutations

import (
  "code.google.com/p/goprotobuf/proto"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
)

type MutateFunc func(duration uint64, input chan *openinstrument_proto.Value, output chan *openinstrument_proto.Value)

func Mean(duration uint64, input chan *openinstrument_proto.Value, output chan *openinstrument_proto.Value) {
  var sum float64
  var count uint64
  var first_timestamp uint64
  var last_timestamp uint64
  first := true
  for v := range input {
    if duration == 0 {
      output <- v
      continue
    }

    if first {
      first_timestamp = v.GetTimestamp()
      first = false
    }

    if v.GetTimestamp() - first_timestamp > duration {
      mean := sum / float64(count)
      output <- &openinstrument_proto.Value{
        Timestamp:   proto.Uint64(last_timestamp),
        DoubleValue: proto.Float64(mean),
      }
      first_timestamp = v.GetTimestamp()
      count = 0
      sum = 0
    }
    sum += v.GetDoubleValue()
    count++
    last_timestamp = v.GetTimestamp()
  }
  if count > 0 {
    mean := sum / float64(count)
    output <- &openinstrument_proto.Value{
      Timestamp:   proto.Uint64(last_timestamp),
      DoubleValue: proto.Float64(mean),
    }
  }
  close(output)
}

func SignedRate(duration uint64, input chan *openinstrument_proto.Value, output chan *openinstrument_proto.Value) {
  var last_value float64
  var last_timestamp uint64
  first := true
  for v := range input {
    if v.DoubleValue == nil {
      continue
    }
    if first {
      last_value = v.GetDoubleValue()
      last_timestamp = v.GetTimestamp()
      first = false
      continue
    }
    rate := (v.GetDoubleValue() - last_value) / float64(v.GetTimestamp() - last_timestamp)
    output <- &openinstrument_proto.Value{
      Timestamp:   v.Timestamp,
      DoubleValue: proto.Float64(rate),
    }
    last_value = v.GetDoubleValue()
    last_timestamp = v.GetTimestamp()
  }
  close(output)
}

func Rate(duration uint64, input chan *openinstrument_proto.Value, output chan *openinstrument_proto.Value) {
  newoutput := make(chan *openinstrument_proto.Value)
  go SignedRate(duration, input, newoutput)
  for v := range newoutput {
    if v.GetDoubleValue() >= 0 {
      output <- v
    }
  }
  close(output)
}

// Interpolate takes a counter value sampled at variable intervals and aligns each value with a <duration> ms boundary.
// The initial value is not output unless it appears exactly on a ms boundary.
// Runs of two or more missing values will be filled by NaN
func Interpolate(duration uint64, input chan *openinstrument_proto.Value, output chan *openinstrument_proto.Value) {
  var previous_value *openinstrument_proto.Value = nil
  var timestamp uint64
  for v := range input {
    if previous_value == nil {
      if v.GetTimestamp() % duration == 0 {
        // Value is exactly on a timestamp
        output <- v
        previous_value = v
        timestamp = v.GetTimestamp() + duration
      } else {
        previous_value = v
        previous_value.Timestamp = proto.Uint64(v.GetTimestamp() / duration * duration)
        timestamp = previous_value.GetTimestamp() + duration
      }
      //log.Printf("First value at timestamp %s, next is at %s", time.Unix(int64(v.GetTimestamp() / 1000), 0), time.Unix(int64(timestamp / 1000), 0))
      continue
    }
    //log.Printf("Found value at %s (>= %s)", time.Unix(int64(v.GetTimestamp() / 1000), 0), time.Unix(int64(timestamp / 1000), 0))
    if v.GetTimestamp() >= timestamp {
      // Fill in any missing values before this one
      rate := float64((v.GetDoubleValue() - previous_value.GetDoubleValue()))
      //log.Printf("Current rate is %f", rate)
      for ; timestamp <= v.GetTimestamp(); timestamp += duration {
        pct := float64(timestamp - previous_value.GetTimestamp()) / float64(v.GetTimestamp() - previous_value.GetTimestamp())
        //log.Printf("pct = %d - %d / %d - %d", timestamp, previous_value.GetTimestamp(), v.GetTimestamp(), previous_value.GetTimestamp())
        new_value := previous_value.GetDoubleValue() + (rate * pct)
        //log.Printf("Filling in %f value %f at timestamp %d", pct, new_value, timestamp)
        output <- &openinstrument_proto.Value{
          Timestamp: proto.Uint64(timestamp),
          DoubleValue: proto.Float64(new_value),
        }
      }
      if previous_value.GetTimestamp() < v.GetTimestamp() {
        if v.GetTimestamp() % duration == 0 {
          // Value is exactly on a timestamp
          //log.Printf("Value exactly at timestamp %d", timestamp)
          output <- v
          previous_value = v
          timestamp += duration
        } else {
          previous_value = v
        }
      }
    }
  }
  close(output)
}

func Min(duration uint64, input chan *openinstrument_proto.Value, output chan *openinstrument_proto.Value) {
  var min float64
  var last_timestamp uint64
  first := true
  for v := range input {
    if first {
      last_timestamp = v.GetTimestamp()
      min = v.GetDoubleValue()
      first = false
      continue
    }
    if v.GetTimestamp() >= last_timestamp + duration {
      output <- &openinstrument_proto.Value{
        Timestamp:   proto.Uint64(v.GetTimestamp()),
        DoubleValue: proto.Float64(min),
      }
      last_timestamp = v.GetTimestamp()
      first = true
    }
    if first || v.GetDoubleValue() < min {
      min = v.GetDoubleValue()
    }
    first = false
  }
  close(output)
}

func Max(duration uint64, input chan *openinstrument_proto.Value, output chan *openinstrument_proto.Value) {
  var max float64
  var last_timestamp uint64
  first := true
  for v := range input {
    if first {
      last_timestamp = v.GetTimestamp()
      max = v.GetDoubleValue()
      first = false
      continue
    }
    if v.GetTimestamp() >= last_timestamp + duration {
      output <- &openinstrument_proto.Value{
        Timestamp:   proto.Uint64(v.GetTimestamp()),
        DoubleValue: proto.Float64(max),
      }
      last_timestamp = v.GetTimestamp()
      first = true
    }
    if first || v.GetDoubleValue() > max {
      max = v.GetDoubleValue()
    }
    first = false
  }
  close(output)
}

/*
func StdDev(duration uint64, input chan *openinstrument_proto.Value, output chan *openinstrument_proto.Value) {
  var sum float64
  var count uint64
  var last_timestamp uint64
  for _, v := range values {
    sum += v.GetDoubleValue()
    count++
    last_timestamp = v.GetTimestamp()
  }
  mean := float64(sum / float64(count))
  var square_sum float64
  for _, v := range values {
    diff := v.GetDoubleValue() - mean
    square_sum += math.Pow(diff, 2)
  }

  ret := make([]*openinstrument_proto.Value, 1)
  ret[0] = &openinstrument_proto.Value{
    Timestamp:   proto.Uint64(last_timestamp),
    DoubleValue: proto.Float64(math.Sqrt(square_sum / float64(count))),
  }
  return ret
}

func Max(duration uint64, input chan *openinstrument_proto.Value, output chan *openinstrument_proto.Value) {
  var max float64
  var last_timestamp uint64
  for _, v := range values {
    if v.GetDoubleValue() > max {
      max = v.GetDoubleValue()
    }
    last_timestamp = v.GetTimestamp()
  }
  ret := make([]*openinstrument_proto.Value, 1)
  ret[0] = &openinstrument_proto.Value{
    Timestamp:   proto.Uint64(last_timestamp),
    DoubleValue: proto.Float64(max),
  }
  return ret
}

func Delta(duration uint64, input chan *openinstrument_proto.Value, output chan *openinstrument_proto.Value) {
  var first_value float64
  var last_value float64
  var first_timestamp uint64
  var last_timestamp uint64
  first := true
  for _, v := range values {
    if first {
      first_value = v.GetDoubleValue()
      first_timestamp = v.GetTimestamp()
      first = false
    }
    last_value = v.GetDoubleValue()
    last_timestamp = v.GetTimestamp()
  }

  value := (last_value - first_value) / float64(last_timestamp-first_timestamp)
  ret := make([]*openinstrument_proto.Value, 0)
  if math.IsNaN(value) {
    return ret
  }
  ret = append(ret, &openinstrument_proto.Value{
    Timestamp:   proto.Uint64(last_timestamp),
    DoubleValue: proto.Float64(value),
  })
  return ret
}

func Rate(duration uint64, input chan *openinstrument_proto.Value, output chan *openinstrument_proto.Value) {
  ret := SignedRate(values)
  if len(ret) == 0 || ret[0].GetDoubleValue() < 0 || math.IsNaN(ret[0].GetDoubleValue()) {
    return make([]*openinstrument_proto.Value, 0)
  }
  return ret
}

func SignedRate(duration uint64, input chan *openinstrument_proto.Value, output chan *openinstrument_proto.Value) {
  var first_value float64
  var last_value float64
  var first_timestamp uint64
  var last_timestamp uint64
  first := true
  for _, v := range values {
    if first {
      first_value = v.GetDoubleValue()
      first_timestamp = v.GetTimestamp()
      first = false
    }
    last_value = v.GetDoubleValue()
    last_timestamp = v.GetTimestamp()
  }

  value := (last_value - first_value) / float64(last_timestamp-first_timestamp)
  ret := make([]*openinstrument_proto.Value, 0)
  if math.IsNaN(value) {
    return ret
  }
  ret = append(ret, &openinstrument_proto.Value{
    Timestamp:   proto.Uint64(last_timestamp),
    DoubleValue: proto.Float64(value),
  })
  return ret
}

func Latest(duration uint64, input chan *openinstrument_proto.Value, output chan *openinstrument_proto.Value) {
  var last_value float64
  var last_timestamp uint64
  for _, v := range values {
    last_timestamp = v.GetTimestamp()
    last_value = v.GetDoubleValue()
  }
  ret := make([]*openinstrument_proto.Value, 1)
  ret[0] = &openinstrument_proto.Value{
    Timestamp:   proto.Uint64(last_timestamp),
    DoubleValue: proto.Float64(last_value),
  }
  return ret
}
*/

// MutateValues performs a mutation on an input channel of Values.
// The mutation will be performed on values every <duration> ms apart, with the output timestamp set to the last
// timestamp of the sequence.
func MutateValues(duration uint64, input chan *openinstrument_proto.Value, f MutateFunc) chan *openinstrument_proto.Value {
  c := make(chan *openinstrument_proto.Value)
  go func() {
    mutate_input := make(chan *openinstrument_proto.Value)
    go f(duration, mutate_input, c)
    for value := range input {
      mutate_input <- value
    }
    close(mutate_input)
  }()
  return c
}
