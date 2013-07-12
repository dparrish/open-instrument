package openinstrument

import (
  "code.google.com/p/goprotobuf/proto"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
  "code.google.com/p/open-instrument/variable"
  "time"
)

func NewVariableFromString(textvar string) *variable.Variable {
  return variable.NewFromString(textvar)
}

func NewVariableFromProto(p *openinstrument_proto.StreamVariable) *variable.Variable {
  return variable.NewFromProto(p)
}

type Timer struct {
  t          *openinstrument_proto.Timer
  start_time time.Time
  name       string
}

func NewTimer(name string, t *openinstrument_proto.Timer) *Timer {
  return &Timer{
    start_time: time.Now(),
    t:          t,
    name:       name,
  }
}

func (this *Timer) Stop() uint64 {
  duration := time.Since(this.start_time)
  if this.t != nil {
    this.t.Ms = proto.Uint64(uint64(duration.Nanoseconds() / 1000000))
    if this.name != "" {
      this.t.Name = &this.name
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

// ValueStreamWriter returns a channel that appends values to the supplied ValueStream
// No effort is made to ensure that the ValueStream contains sorted Values
func ValueStreamWriter(stream *openinstrument_proto.ValueStream) chan *openinstrument_proto.Value {
  c := make(chan *openinstrument_proto.Value)
  go func() {
    for value := range c {
      stream.Value = append(stream.Value, value)
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
  }()
  return c
}

// MergeValueStreams merges multiple ValueStreams, returning channel producing sorted Values.
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
