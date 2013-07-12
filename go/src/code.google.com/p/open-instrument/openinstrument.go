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
    t: t,
    name: name,
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
