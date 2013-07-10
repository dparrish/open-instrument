package openinstrument

import "code.google.com/p/open-instrument/variable"
import openinstrument_proto "code.google.com/p/open-instrument/proto"

func NewVariableFromString(textvar string) *variable.Variable {
  return variable.NewFromString(textvar)
}

func NewVariableFromProto(p *openinstrument_proto.StreamVariable) *variable.Variable {
  return variable.NewFromProto(p)
}
