package openinstrument

import (
  "code.google.com/p/goprotobuf/proto"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
  "code.google.com/p/open-instrument/variable"
  "errors"
  "fmt"
  "os"
  "sort"
  "time"
)

func NewVariableFromString(textvar string) *variable.Variable {
  return variable.NewFromString(textvar)
}

func NewVariableFromProto(p *openinstrument_proto.StreamVariable) *variable.Variable {
  return variable.NewFromProto(p)
}

type Timer struct {
  t          *openinstrument_proto.LogMessage
  start_time time.Time
  message    string
}

func NewTimer(message string, t *openinstrument_proto.LogMessage) *Timer {
  return &Timer{
    start_time: time.Now(),
    t:          t,
    message:    message,
  }
}

func (this *Timer) Stop() uint64 {
  duration := time.Since(this.start_time)
  if this.t != nil {
    this.t.Timestamp = proto.Uint64(uint64(duration.Nanoseconds() / 1000000))
    if this.message != "" {
      this.t.Message = &this.message
    }
  }
  return uint64(duration.Nanoseconds() / 1000000)
}

type Semaphore chan bool

// acquire n resources
func (s Semaphore) P(n int) {
  for i := 0; i < n; i++ {
    s <- true
  }
}

// release n resources
func (s Semaphore) V(n int) {
  for i := 0; i < n; i++ {
    <-s
  }
}

func (s Semaphore) Lock() {
  s.P(1)
}

func (s Semaphore) Unlock() {
  s.V(1)
}

/* signal-wait */

func (s Semaphore) Signal() {
  s.V(1)
}

func (s Semaphore) Wait(n int) {
  s.P(n)
}

// ValueStreamWriter returns a channel that appends values to the supplied ValueStream, performing run-length-encoding.
// No effort is made to ensure that the ValueStream contains sorted Values
func ValueStreamWriter(stream *openinstrument_proto.ValueStream) chan *openinstrument_proto.Value {
  c := make(chan *openinstrument_proto.Value)
  go func() {
    for value := range c {
      if len(stream.Value) > 0 {
        last := stream.Value[len(stream.Value)-1]
        if (last.GetStringValue() != "" && last.GetStringValue() == value.GetStringValue()) ||
          (last.GetDoubleValue() == value.GetDoubleValue()) {
          if value.GetEndTimestamp() > 0 {
            last.EndTimestamp = value.EndTimestamp
          } else {
            last.EndTimestamp = value.Timestamp
          }
        } else {
          stream.Value = append(stream.Value, value)
        }
      } else {
        stream.Value = append(stream.Value, value)
      }
    }
  }()
  return c
}

// ValueStreamReader returns a channel producing Values from the supplied ValueStream
func ValueStreamReader(stream *openinstrument_proto.ValueStream) chan *openinstrument_proto.Value {
  c := make(chan *openinstrument_proto.Value)
  go func() {
    for _, value := range stream.Value {
      c <- value
    }
    close(c)
  }()
  return c
}

// MergeValueStreams merges multiple ValueStreams, returning a channel producing sorted Values.
func MergeValueStreams(streams []*openinstrument_proto.ValueStream) chan *openinstrument_proto.Value {
  c := make(chan *openinstrument_proto.Value)
  n := len(streams)
  go func() {
    indexes := make([]int, n)
    for {
      var min_timestamp uint64
      var min_stream *openinstrument_proto.ValueStream
      var min_value *openinstrument_proto.Value
      for i := 0; i < n; i++ {
        if indexes[i] >= len(streams[i].Value) {
          continue
        }
        v := streams[i].Value[indexes[i]]
        if min_stream == nil || v.GetTimestamp() < min_timestamp {
          min_timestamp = v.GetTimestamp()
          min_stream = streams[i]
          min_value = v
          indexes[i]++
        }
      }
      if min_value == nil {
        break
      }
      c <- min_value
    }
    close(c)
  }()
  return c
}

func MergeStreamsBy(streams []*openinstrument_proto.ValueStream, by string) chan []*openinstrument_proto.ValueStream {
  c := make(chan []*openinstrument_proto.ValueStream)
  go func() {
    unique_vars := make(map[string]bool)
    unique_labels := make(map[string]bool)
    for _, stream := range streams {
      v := variable.NewFromProto(stream.Variable)
      unique_vars[v.Variable] = true
      label_value, ok := v.Labels[by]
      if !ok {
        unique_labels[""] = true
      } else {
        unique_labels[label_value] = true
      }
    }
    for varname := range unique_vars {
      v := variable.NewFromString(varname)
      if by == "" {
        output := make([]*openinstrument_proto.ValueStream, 0)
        for _, stream := range streams {
          testvar := variable.NewFromProto(stream.Variable)
          if testvar.Variable != v.Variable {
            continue
          }
          output = append(output, stream)
        }
        if len(output) > 0 {
          c <- output
        }
      } else {
        for labelvalue := range unique_labels {
          output := make([]*openinstrument_proto.ValueStream, 0)
          for _, stream := range streams {
            testvar := variable.NewFromProto(stream.Variable)
            if testvar.Variable != v.Variable {
              continue
            }
            value, ok := testvar.Labels[by]
            if !ok {
              continue
            }
            if value != labelvalue {
              continue
            }
            output = append(output, stream)
          }
          if len(output) > 0 {
            c <- output
          }
        }
      }
    }
    close(c)
  }()
  return c
}

type valueStreamChannelList struct {
  input    chan *openinstrument_proto.Value
  channels []chan *openinstrument_proto.Value
}

func (this *valueStreamChannelList) Add(c chan *openinstrument_proto.Value) {
  this.channels = append(this.channels, c)
}

func (this *valueStreamChannelList) Last() chan *openinstrument_proto.Value {
  return this.channels[len(this.channels)-1]
}

func ValueStreamChannelList(initial chan *openinstrument_proto.Value) *valueStreamChannelList {
  this := new(valueStreamChannelList)
  this.channels = make([]chan *openinstrument_proto.Value, 0)
  this.channels = append(this.channels, initial)
  return this
}

func Readdirnames(directory string) ([]string, error) {
  dir, err := os.Open(directory)
  if err != nil {
    return nil, errors.New(fmt.Sprintf("Can't open %s for readdir: %s", directory, err))
  }
  defer dir.Close()
  names, err := dir.Readdirnames(0)
  if err != nil {
    return nil, errors.New(fmt.Sprintf("Can't read file names in %s: %s", directory, err))
  }
  sort.Strings(names)
  return names, nil
}
