package mutations

import (
  "code.google.com/p/goprotobuf/proto"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
  "math"
)

type MutateFunc func(values []*openinstrument_proto.Value) []*openinstrument_proto.Value

func Mean(values []*openinstrument_proto.Value) []*openinstrument_proto.Value {
  var sum float64
  var count uint64
  var last_timestamp uint64
  for _, v := range values {
    sum += v.GetDoubleValue()
    count++
    last_timestamp = v.GetTimestamp()
  }
  ret := make([]*openinstrument_proto.Value, 1)
  ret[0] = &openinstrument_proto.Value{
    Timestamp:   proto.Uint64(last_timestamp),
    DoubleValue: proto.Float64(sum / float64(count)),
  }
  return ret
}

func StdDev(values []*openinstrument_proto.Value) []*openinstrument_proto.Value {
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

func Max(values []*openinstrument_proto.Value) []*openinstrument_proto.Value {
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

func Min(values []*openinstrument_proto.Value) []*openinstrument_proto.Value {
  var min float64
  var last_timestamp uint64
  for _, v := range values {
    if min == 0 || v.GetDoubleValue() < min {
      min = v.GetDoubleValue()
    }
    last_timestamp = v.GetTimestamp()
  }
  ret := make([]*openinstrument_proto.Value, 1)
  ret[0] = &openinstrument_proto.Value{
    Timestamp:   proto.Uint64(last_timestamp),
    DoubleValue: proto.Float64(min),
  }
  return ret
}

func Rate(values []*openinstrument_proto.Value) []*openinstrument_proto.Value {
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

  ret := make([]*openinstrument_proto.Value, 1)
  ret[0] = &openinstrument_proto.Value{
    Timestamp:   proto.Uint64(last_timestamp),
    DoubleValue: proto.Float64((last_value - first_value) / float64(last_timestamp - first_timestamp)),
  }
  return ret
}


// MutateValues performs a mutation on an input channel of Values.
// The mutation will be performed on values every <duration> ms apart, with the output timestamp set to the last
// timestamp of the sequence.
func MutateValues(duration uint64, input chan *openinstrument_proto.Value, f MutateFunc) chan *openinstrument_proto.Value {
  c := make(chan *openinstrument_proto.Value)
  var first_timestamp uint64
  values := make([]*openinstrument_proto.Value, 0)
  var last_timestamp uint64
  go func() {
    for {
      value, ok := <-input
      if ok {
        if first_timestamp == 0 {
          first_timestamp = value.GetTimestamp()
        }
      }
      if !ok || value.GetTimestamp() >= first_timestamp+duration {
        // Got enough data
        if len(values) > 0 {
          for _, newvalue := range f(values) {
            c <- newvalue
          }
          values = make([]*openinstrument_proto.Value, 0)
          first_timestamp = 0
        }
      }
      if !ok {
        break
      }
      values = append(values, value)
      last_timestamp = value.GetTimestamp()
    }
    close(c)
  }()
  return c
}
