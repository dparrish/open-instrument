package variable

import (
  "code.google.com/p/goprotobuf/proto"
  "testing"
  "code.google.com/p/open-instrument/variable"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
  . "launchpad.net/gocheck"
)

// Hook up gocheck into the "go test" runner.
func Test(t *testing.T) { TestingT(t) }

type MySuite struct{}
var _ = Suite(&MySuite{})

func (s *MySuite) TestVariableCreation(c *C) {
  newvar := variable.NewFromString("/openinstrument/test{label1=value1,label2=spaced value}")
  c.Check(newvar.String(), Equals, "/openinstrument/test{label1=value1,label2=spaced value}")

  labels := make([]*openinstrument_proto.Label, 2)
  labels[0] = &openinstrument_proto.Label{Label: proto.String("label1"), Value: proto.String("value1")}
  labels[1] = &openinstrument_proto.Label{Label: proto.String("label2"), Value: proto.String("spaced value")}
  newvar = variable.NewFromProto(&openinstrument_proto.StreamVariable{
    Name: proto.String("/openinstrument/test"),
    Label: labels,
  })
  c.Check(newvar.String(), Equals, "/openinstrument/test{label1=value1,label2=spaced value}")

  p := newvar.AsProto()
  newvar.ParseFromProto(p)
  c.Check(newvar.String(), Equals, "/openinstrument/test{label1=value1,label2=spaced value}")
}

func (s *MySuite) TestToProto(c *C) {
  newvar := variable.NewFromString("/openinstrument/test{label1=value1,label2=spaced value}")
  p := newvar.AsProto()
  newvar.ParseFromProto(p)
  c.Check(newvar.String(), Equals, "/openinstrument/test{label1=value1,label2=spaced value}")
}

func (s *MySuite) TestMatch(c *C) {
  var1 := variable.NewFromString("/openinstrument/test{label1=value1,label2=spaced value}")
  c.Check(var1.Match(variable.NewFromString("/openinstrument/*")), Equals, true)
  c.Check(var1.Match(variable.NewFromString("/openinstrx/*")), Equals, false)
  c.Check(var1.Match(variable.NewFromString("/openinstrument/")), Equals, false)
  c.Check(var1.Match(variable.NewFromString("/openinstrument/test")), Equals, true)
  c.Check(var1.Match(variable.NewFromString("/openinstrument/test{label1=value1}")), Equals, true)
  c.Check(var1.Match(variable.NewFromString("/openinstrument/test{label1=value2}")), Equals, false)
  c.Check(var1.Match(variable.NewFromString("/openinstrument/test{label1=*}")), Equals, true)
  c.Check(var1.Match(variable.NewFromString("/openinstrument/test{label1=}")), Equals, false)
  c.Check(var1.Match(variable.NewFromString("/openinstrument/test{label3=*}")), Equals, false)
}
