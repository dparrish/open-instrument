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
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(0), DoubleValue: proto.Float64(20)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(60), DoubleValue: proto.Float64(40)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(120), DoubleValue: proto.Float64(60)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(180), DoubleValue: proto.Float64(80)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(240), DoubleValue: proto.Float64(100)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(300), DoubleValue: proto.Float64(120)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(330), DoubleValue: proto.Float64(130)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(360), DoubleValue: proto.Float64(140)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(420), DoubleValue: proto.Float64(160)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(480), DoubleValue: proto.Float64(180)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(540), DoubleValue: proto.Float64(200)}
    close(ic)
  }
  write_offset_values := func(ic chan *openinstrument_proto.Value) {
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(0), DoubleValue: proto.Float64(20)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(61), DoubleValue: proto.Float64(40)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(122), DoubleValue: proto.Float64(60)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(185), DoubleValue: proto.Float64(80)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(241), DoubleValue: proto.Float64(100)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(299), DoubleValue: proto.Float64(122)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(330), DoubleValue: proto.Float64(132)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(359), DoubleValue: proto.Float64(140)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(421), DoubleValue: proto.Float64(160)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(488), DoubleValue: proto.Float64(180)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(540), DoubleValue: proto.Float64(200)}
    ic <- &openinstrument_proto.Value{Timestamp: proto.Uint64(975), DoubleValue: proto.Float64(275)}
    close(ic)
  }

  log.Println("Mean")
  ic := make(chan *openinstrument_proto.Value, 100)
  oc := mutations.MutateValues(300, ic, mutations.Mean)
  go write_values(ic)
  value := <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(70))
  value = <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(162))

  log.Println("Interpolate")
  ic = make(chan *openinstrument_proto.Value, 100)
  oc = mutations.MutateValues(300, ic, mutations.Interpolate)
  go write_offset_values(ic)
  value = <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(20))
  c.Check(value.GetTimestamp(), Equals, uint64(0))
  value = <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(121.81818181818181))
  c.Check(value.GetTimestamp(), Equals, uint64(300))
  value = <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(191.86046511627907))
  c.Check(value.GetTimestamp(), Equals, uint64(600))
  value = <-oc
  c.Check(value.GetDoubleValue(), Equals, float64(258.37209302325584))
  c.Check(value.GetTimestamp(), Equals, uint64(900))
  for v := range oc {
    log.Println(v)
  }

  /*
     log.Println("Signed Rate")
     ic = make(chan *openinstrument_proto.Value)
     oc = mutations.MutateValues(300, ic, mutations.SignedRate)
     go write_values(ic)
     value = <-oc
     c.Check(value.GetDoubleValue(), Equals, float64(1.0/3.0))
     value = <-oc
     c.Check(value.GetDoubleValue(), Equals, float64(1.0/3.0))

     log.Println("Standard Deviation")
     ic = make(chan *openinstrument_proto.Value)
     oc = mutations.MutateValues(60, ic, mutations.StdDev)
     go write_values(ic)
     value = <-oc
     c.Check(value.GetDoubleValue(), Equals, float64(2))
     value = <-oc
     c.Check(value.GetDoubleValue(), Equals, float64(47.5))

     log.Println("Max")
     ic = make(chan *openinstrument_proto.Value)
     oc = mutations.MutateValues(60, ic, mutations.Max)
     go write_values(ic)
     value = <-oc
     c.Check(value.GetDoubleValue(), Equals, float64(9))
     value = <-oc
     c.Check(value.GetDoubleValue(), Equals, float64(99))

     log.Println("Min")
     ic = make(chan *openinstrument_proto.Value)
     oc = mutations.MutateValues(60, ic, mutations.Min)
     go write_values(ic)
     value = <-oc
     c.Check(value.GetDoubleValue(), Equals, float64(2))
     value = <-oc
     c.Check(value.GetDoubleValue(), Equals, float64(4))

     log.Println("Rate")
     ic = make(chan *openinstrument_proto.Value)
     oc = mutations.MutateValues(60, ic, mutations.Rate)
     go write_values(ic)
     value = <-oc
     c.Check(value.GetDoubleValue(), Equals, float64(1))
     value = <-oc
     c.Check(value.GetDoubleValue(), Equals, float64(1.7592592592592593))
  */
}
