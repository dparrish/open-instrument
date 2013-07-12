package openinstrument

import (
  "fmt"
  "time"
)

type DurationTimer struct {
  name       string
  start_time time.Time
  end_time   time.Time
  total_time time.Duration
  running    bool
}

func NewNamedDurationTimer(name string) *DurationTimer {
  return &DurationTimer{
    name: name,
  }
}

func NewDurationTimer() *DurationTimer {
  return &DurationTimer{}
}

func (this *DurationTimer) Start() {
  if !this.running {
    this.start_time = time.Now()
    this.running = true
  }
}

func (this *DurationTimer) Stop() {
  if this.running {
    this.end_time = time.Now()
    d := this.end_time.Sub(this.start_time)
    this.total_time = time.Duration(d.Nanoseconds() + this.total_time.Nanoseconds())
  }
}

func (this *DurationTimer) Duration() time.Duration {
  if this.running {
    this.Stop()
  }
  return this.total_time
}

func (this *DurationTimer) String() string {
  if this.name != "" {
    return fmt.Sprintf("%s: %s", this.name, this.Duration())
  }
  return this.Duration().String()
}
