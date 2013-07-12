package mutations

import (
  "code.google.com/p/goprotobuf/proto"
  "code.google.com/p/open-instrument/mutations"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
  . "launchpad.net/gocheck"
  "log"
  "testing"
)

// Hook up gocheck into the "go test" runner.
func Test(t *testing.T) { TestingT(t) }

type MySuite struct{}

var _ = Suite(&MySuite{})

func (s *MySuite) TestMutations(c *C) {
  write_values := func(ic chan *openinstrument_proto.Value) {
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(1), DoubleValue: proto.Float64(2)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(2), DoubleValue: proto.Float64(4)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(3), DoubleValue: proto.Float64(4)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(3), DoubleValue: proto.Float64(4)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(5), DoubleValue: proto.Float64(5)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(6), DoubleValue: proto.Float64(5)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(7), DoubleValue: proto.Float64(7)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(8), DoubleValue: proto.Float64(9)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(101), DoubleValue: proto.Float64(4)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(155), DoubleValue: proto.Float64(99)}
    close(ic)
  }

  log.Println("Mean")
  ic := make(chan *openinstrument_proto.Value)
  oc := mutations.MutateValues(100, ic, mutations.Mean)
  go write_values(ic)
  value := <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(5))
  value = <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(51.5))

  log.Println("Standard Deviation")
  ic = make(chan *openinstrument_proto.Value)
  oc = mutations.MutateValues(100, ic, mutations.StdDev)
  go write_values(ic)
  value = <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(2))
  value = <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(47.5))

  log.Println("Max")
  ic = make(chan *openinstrument_proto.Value)
  oc = mutations.MutateValues(100, ic, mutations.Max)
  go write_values(ic)
  value = <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(9))
  value = <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(99))

  log.Println("Min")
  ic = make(chan *openinstrument_proto.Value)
  oc = mutations.MutateValues(100, ic, mutations.Min)
  go write_values(ic)
  value = <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(2))
  value = <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(4))

  log.Println("Rate")
  ic = make(chan *openinstrument_proto.Value)
  oc = mutations.MutateValues(100, ic, mutations.Rate)
  go write_values(ic)
  value = <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(1))
  value = <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(1.7592592592592593))
}
