// Code generated by protoc-gen-go.
// source: openinstrument.proto
// DO NOT EDIT!

package openinstrument_proto

import proto "code.google.com/p/goprotobuf/proto"
import json "encoding/json"
import math "math"

// Reference proto, json, and math imports to suppress error if they are not otherwise used.
var _ = proto.Marshal
var _ = &json.SyntaxError{}
var _ = math.Inf

type StreamVariable_ValueType int32

const (
  StreamVariable_UNKNOWN StreamVariable_ValueType = 0
  StreamVariable_GAUGE   StreamVariable_ValueType = 1
  StreamVariable_RATE    StreamVariable_ValueType = 2
)

var StreamVariable_ValueType_name = map[int32]string{
  0: "UNKNOWN",
  1: "GAUGE",
  2: "RATE",
}
var StreamVariable_ValueType_value = map[string]int32{
  "UNKNOWN": 0,
  "GAUGE":   1,
  "RATE":    2,
}

func (x StreamVariable_ValueType) Enum() *StreamVariable_ValueType {
  p := new(StreamVariable_ValueType)
  *p = x
  return p
}
func (x StreamVariable_ValueType) String() string {
  return proto.EnumName(StreamVariable_ValueType_name, int32(x))
}
func (x StreamVariable_ValueType) MarshalJSON() ([]byte, error) {
  return json.Marshal(x.String())
}
func (x *StreamVariable_ValueType) UnmarshalJSON(data []byte) error {
  value, err := proto.UnmarshalJSONEnum(StreamVariable_ValueType_value, data, "StreamVariable_ValueType")
  if err != nil {
    return err
  }
  *x = StreamVariable_ValueType(value)
  return nil
}

type StreamMutation_SampleType int32

const (
  StreamMutation_NONE        StreamMutation_SampleType = 0
  StreamMutation_AVERAGE     StreamMutation_SampleType = 1
  StreamMutation_MAX         StreamMutation_SampleType = 2
  StreamMutation_MIN         StreamMutation_SampleType = 3
  StreamMutation_RATE        StreamMutation_SampleType = 4
  StreamMutation_RATE_SIGNED StreamMutation_SampleType = 5
  StreamMutation_DELTA       StreamMutation_SampleType = 6
  StreamMutation_LATEST      StreamMutation_SampleType = 7
)

var StreamMutation_SampleType_name = map[int32]string{
  0: "NONE",
  1: "AVERAGE",
  2: "MAX",
  3: "MIN",
  4: "RATE",
  5: "RATE_SIGNED",
  6: "DELTA",
  7: "LATEST",
}
var StreamMutation_SampleType_value = map[string]int32{
  "NONE":        0,
  "AVERAGE":     1,
  "MAX":         2,
  "MIN":         3,
  "RATE":        4,
  "RATE_SIGNED": 5,
  "DELTA":       6,
  "LATEST":      7,
}

func (x StreamMutation_SampleType) Enum() *StreamMutation_SampleType {
  p := new(StreamMutation_SampleType)
  *p = x
  return p
}
func (x StreamMutation_SampleType) String() string {
  return proto.EnumName(StreamMutation_SampleType_name, int32(x))
}
func (x StreamMutation_SampleType) MarshalJSON() ([]byte, error) {
  return json.Marshal(x.String())
}
func (x *StreamMutation_SampleType) UnmarshalJSON(data []byte) error {
  value, err := proto.UnmarshalJSONEnum(StreamMutation_SampleType_value, data, "StreamMutation_SampleType")
  if err != nil {
    return err
  }
  *x = StreamMutation_SampleType(value)
  return nil
}

type StreamAggregation_AggregateType int32

const (
  StreamAggregation_AVERAGE StreamAggregation_AggregateType = 0
  StreamAggregation_MAX     StreamAggregation_AggregateType = 1
  StreamAggregation_MIN     StreamAggregation_AggregateType = 2
  StreamAggregation_MEDIAN  StreamAggregation_AggregateType = 3
  StreamAggregation_SUM     StreamAggregation_AggregateType = 4
)

var StreamAggregation_AggregateType_name = map[int32]string{
  0: "AVERAGE",
  1: "MAX",
  2: "MIN",
  3: "MEDIAN",
  4: "SUM",
}
var StreamAggregation_AggregateType_value = map[string]int32{
  "AVERAGE": 0,
  "MAX":     1,
  "MIN":     2,
  "MEDIAN":  3,
  "SUM":     4,
}

func (x StreamAggregation_AggregateType) Enum() *StreamAggregation_AggregateType {
  p := new(StreamAggregation_AggregateType)
  *p = x
  return p
}
func (x StreamAggregation_AggregateType) String() string {
  return proto.EnumName(StreamAggregation_AggregateType_name, int32(x))
}
func (x StreamAggregation_AggregateType) MarshalJSON() ([]byte, error) {
  return json.Marshal(x.String())
}
func (x *StreamAggregation_AggregateType) UnmarshalJSON(data []byte) error {
  value, err := proto.UnmarshalJSONEnum(StreamAggregation_AggregateType_value, data, "StreamAggregation_AggregateType")
  if err != nil {
    return err
  }
  *x = StreamAggregation_AggregateType(value)
  return nil
}

type RetentionPolicyItem_Target int32

const (
  RetentionPolicyItem_KEEP RetentionPolicyItem_Target = 1
  RetentionPolicyItem_DROP RetentionPolicyItem_Target = 2
)

var RetentionPolicyItem_Target_name = map[int32]string{
  1: "KEEP",
  2: "DROP",
}
var RetentionPolicyItem_Target_value = map[string]int32{
  "KEEP": 1,
  "DROP": 2,
}

func (x RetentionPolicyItem_Target) Enum() *RetentionPolicyItem_Target {
  p := new(RetentionPolicyItem_Target)
  *p = x
  return p
}
func (x RetentionPolicyItem_Target) String() string {
  return proto.EnumName(RetentionPolicyItem_Target_name, int32(x))
}
func (x RetentionPolicyItem_Target) MarshalJSON() ([]byte, error) {
  return json.Marshal(x.String())
}
func (x *RetentionPolicyItem_Target) UnmarshalJSON(data []byte) error {
  value, err := proto.UnmarshalJSONEnum(RetentionPolicyItem_Target_value, data, "RetentionPolicyItem_Target")
  if err != nil {
    return err
  }
  *x = RetentionPolicyItem_Target(value)
  return nil
}

type StoreServer_State int32

const (
  StoreServer_UNKNOWN  StoreServer_State = 0
  StoreServer_STARTING StoreServer_State = 1
  StoreServer_LOADING  StoreServer_State = 2
  StoreServer_RUNNING  StoreServer_State = 3
  StoreServer_READONLY StoreServer_State = 4
  StoreServer_DRAINING StoreServer_State = 5
  StoreServer_LAMEDUCK StoreServer_State = 6
  StoreServer_SHUTDOWN StoreServer_State = 7
)

var StoreServer_State_name = map[int32]string{
  0: "UNKNOWN",
  1: "STARTING",
  2: "LOADING",
  3: "RUNNING",
  4: "READONLY",
  5: "DRAINING",
  6: "LAMEDUCK",
  7: "SHUTDOWN",
}
var StoreServer_State_value = map[string]int32{
  "UNKNOWN":  0,
  "STARTING": 1,
  "LOADING":  2,
  "RUNNING":  3,
  "READONLY": 4,
  "DRAINING": 5,
  "LAMEDUCK": 6,
  "SHUTDOWN": 7,
}

func (x StoreServer_State) Enum() *StoreServer_State {
  p := new(StoreServer_State)
  *p = x
  return p
}
func (x StoreServer_State) String() string {
  return proto.EnumName(StoreServer_State_name, int32(x))
}
func (x StoreServer_State) MarshalJSON() ([]byte, error) {
  return json.Marshal(x.String())
}
func (x *StoreServer_State) UnmarshalJSON(data []byte) error {
  value, err := proto.UnmarshalJSONEnum(StoreServer_State_value, data, "StoreServer_State")
  if err != nil {
    return err
  }
  *x = StoreServer_State(value)
  return nil
}

type Label struct {
  Label            *string `protobuf:"bytes,1,req,name=label" json:"label,omitempty"`
  Value            *string `protobuf:"bytes,2,opt,name=value" json:"value,omitempty"`
  XXX_unrecognized []byte  `json:"-"`
}

func (m *Label) Reset()         { *m = Label{} }
func (m *Label) String() string { return proto.CompactTextString(m) }
func (*Label) ProtoMessage()    {}

func (m *Label) GetLabel() string {
  if m != nil && m.Label != nil {
    return *m.Label
  }
  return ""
}

func (m *Label) GetValue() string {
  if m != nil && m.Value != nil {
    return *m.Value
  }
  return ""
}

type StreamVariable struct {
  Name             *string                   `protobuf:"bytes,1,req,name=name" json:"name,omitempty"`
  Label            []*Label                  `protobuf:"bytes,2,rep,name=label" json:"label,omitempty"`
  Type             *StreamVariable_ValueType `protobuf:"varint,3,opt,name=type,enum=openinstrument.proto.StreamVariable_ValueType" json:"type,omitempty"`
  XXX_unrecognized []byte                    `json:"-"`
}

func (m *StreamVariable) Reset()         { *m = StreamVariable{} }
func (m *StreamVariable) String() string { return proto.CompactTextString(m) }
func (*StreamVariable) ProtoMessage()    {}

func (m *StreamVariable) GetName() string {
  if m != nil && m.Name != nil {
    return *m.Name
  }
  return ""
}

func (m *StreamVariable) GetLabel() []*Label {
  if m != nil {
    return m.Label
  }
  return nil
}

func (m *StreamVariable) GetType() StreamVariable_ValueType {
  if m != nil && m.Type != nil {
    return *m.Type
  }
  return 0
}

type StreamMutation struct {
  SampleType        *StreamMutation_SampleType `protobuf:"varint,1,req,name=sample_type,enum=openinstrument.proto.StreamMutation_SampleType" json:"sample_type,omitempty"`
  SampleFrequency   *uint32                    `protobuf:"varint,2,opt,name=sample_frequency" json:"sample_frequency,omitempty"`
  MaxGapInterpolate *uint32                    `protobuf:"varint,3,opt,name=max_gap_interpolate,def=1" json:"max_gap_interpolate,omitempty"`
  XXX_unrecognized  []byte                     `json:"-"`
}

func (m *StreamMutation) Reset()         { *m = StreamMutation{} }
func (m *StreamMutation) String() string { return proto.CompactTextString(m) }
func (*StreamMutation) ProtoMessage()    {}

const Default_StreamMutation_MaxGapInterpolate uint32 = 1

func (m *StreamMutation) GetSampleType() StreamMutation_SampleType {
  if m != nil && m.SampleType != nil {
    return *m.SampleType
  }
  return 0
}

func (m *StreamMutation) GetSampleFrequency() uint32 {
  if m != nil && m.SampleFrequency != nil {
    return *m.SampleFrequency
  }
  return 0
}

func (m *StreamMutation) GetMaxGapInterpolate() uint32 {
  if m != nil && m.MaxGapInterpolate != nil {
    return *m.MaxGapInterpolate
  }
  return Default_StreamMutation_MaxGapInterpolate
}

type StreamAggregation struct {
  Type             *StreamAggregation_AggregateType `protobuf:"varint,1,req,name=type,enum=openinstrument.proto.StreamAggregation_AggregateType" json:"type,omitempty"`
  Label            []string                         `protobuf:"bytes,2,rep,name=label" json:"label,omitempty"`
  SampleInterval   *uint32                          `protobuf:"varint,3,opt,name=sample_interval,def=30000" json:"sample_interval,omitempty"`
  XXX_unrecognized []byte                           `json:"-"`
}

func (m *StreamAggregation) Reset()         { *m = StreamAggregation{} }
func (m *StreamAggregation) String() string { return proto.CompactTextString(m) }
func (*StreamAggregation) ProtoMessage()    {}

const Default_StreamAggregation_SampleInterval uint32 = 30000

func (m *StreamAggregation) GetType() StreamAggregation_AggregateType {
  if m != nil && m.Type != nil {
    return *m.Type
  }
  return 0
}

func (m *StreamAggregation) GetLabel() []string {
  if m != nil {
    return m.Label
  }
  return nil
}

func (m *StreamAggregation) GetSampleInterval() uint32 {
  if m != nil && m.SampleInterval != nil {
    return *m.SampleInterval
  }
  return Default_StreamAggregation_SampleInterval
}

type Value struct {
  Timestamp        *uint64         `protobuf:"varint,1,req,name=timestamp" json:"timestamp,omitempty"`
  DoubleValue      *float64        `protobuf:"fixed64,2,opt,name=double_value" json:"double_value,omitempty"`
  StringValue      *string         `protobuf:"bytes,3,opt,name=string_value" json:"string_value,omitempty"`
  EndTimestamp     *uint64         `protobuf:"varint,4,opt,name=end_timestamp" json:"end_timestamp,omitempty"`
  Variable         *StreamVariable `protobuf:"bytes,5,opt,name=variable" json:"variable,omitempty"`
  XXX_unrecognized []byte          `json:"-"`
}

func (m *Value) Reset()         { *m = Value{} }
func (m *Value) String() string { return proto.CompactTextString(m) }
func (*Value) ProtoMessage()    {}

func (m *Value) GetTimestamp() uint64 {
  if m != nil && m.Timestamp != nil {
    return *m.Timestamp
  }
  return 0
}

func (m *Value) GetDoubleValue() float64 {
  if m != nil && m.DoubleValue != nil {
    return *m.DoubleValue
  }
  return 0
}

func (m *Value) GetStringValue() string {
  if m != nil && m.StringValue != nil {
    return *m.StringValue
  }
  return ""
}

func (m *Value) GetEndTimestamp() uint64 {
  if m != nil && m.EndTimestamp != nil {
    return *m.EndTimestamp
  }
  return 0
}

func (m *Value) GetVariable() *StreamVariable {
  if m != nil {
    return m.Variable
  }
  return nil
}

type ValueStream struct {
  DEPRECATEDStringVariable *string         `protobuf:"bytes,1,opt,name=DEPRECATED_string_variable" json:"DEPRECATED_string_variable,omitempty"`
  Variable                 *StreamVariable `protobuf:"bytes,2,opt,name=variable" json:"variable,omitempty"`
  Value                    []*Value        `protobuf:"bytes,4,rep,name=value" json:"value,omitempty"`
  XXX_unrecognized         []byte          `json:"-"`
}

func (m *ValueStream) Reset()         { *m = ValueStream{} }
func (m *ValueStream) String() string { return proto.CompactTextString(m) }
func (*ValueStream) ProtoMessage()    {}

func (m *ValueStream) GetDEPRECATEDStringVariable() string {
  if m != nil && m.DEPRECATEDStringVariable != nil {
    return *m.DEPRECATEDStringVariable
  }
  return ""
}

func (m *ValueStream) GetVariable() *StreamVariable {
  if m != nil {
    return m.Variable
  }
  return nil
}

func (m *ValueStream) GetValue() []*Value {
  if m != nil {
    return m.Value
  }
  return nil
}

type GetRequest struct {
  DEPRECATEDStringVariable *string              `protobuf:"bytes,1,opt,name=DEPRECATED_string_variable" json:"DEPRECATED_string_variable,omitempty"`
  Variable                 *StreamVariable      `protobuf:"bytes,9,opt,name=variable" json:"variable,omitempty"`
  MinTimestamp             *uint64              `protobuf:"varint,2,opt,name=min_timestamp" json:"min_timestamp,omitempty"`
  MaxTimestamp             *uint64              `protobuf:"varint,3,opt,name=max_timestamp" json:"max_timestamp,omitempty"`
  Mutation                 []*StreamMutation    `protobuf:"bytes,6,rep,name=mutation" json:"mutation,omitempty"`
  Aggregation              []*StreamAggregation `protobuf:"bytes,7,rep,name=aggregation" json:"aggregation,omitempty"`
  MaxVariables             *uint32              `protobuf:"varint,8,opt,name=max_variables,def=100" json:"max_variables,omitempty"`
  Forwarded                *bool                `protobuf:"varint,10,opt,name=forwarded,def=0" json:"forwarded,omitempty"`
  MaxValues                *uint32              `protobuf:"varint,11,opt,name=max_values" json:"max_values,omitempty"`
  XXX_unrecognized         []byte               `json:"-"`
}

func (m *GetRequest) Reset()         { *m = GetRequest{} }
func (m *GetRequest) String() string { return proto.CompactTextString(m) }
func (*GetRequest) ProtoMessage()    {}

const Default_GetRequest_MaxVariables uint32 = 100
const Default_GetRequest_Forwarded bool = false

func (m *GetRequest) GetDEPRECATEDStringVariable() string {
  if m != nil && m.DEPRECATEDStringVariable != nil {
    return *m.DEPRECATEDStringVariable
  }
  return ""
}

func (m *GetRequest) GetVariable() *StreamVariable {
  if m != nil {
    return m.Variable
  }
  return nil
}

func (m *GetRequest) GetMinTimestamp() uint64 {
  if m != nil && m.MinTimestamp != nil {
    return *m.MinTimestamp
  }
  return 0
}

func (m *GetRequest) GetMaxTimestamp() uint64 {
  if m != nil && m.MaxTimestamp != nil {
    return *m.MaxTimestamp
  }
  return 0
}

func (m *GetRequest) GetMutation() []*StreamMutation {
  if m != nil {
    return m.Mutation
  }
  return nil
}

func (m *GetRequest) GetAggregation() []*StreamAggregation {
  if m != nil {
    return m.Aggregation
  }
  return nil
}

func (m *GetRequest) GetMaxVariables() uint32 {
  if m != nil && m.MaxVariables != nil {
    return *m.MaxVariables
  }
  return Default_GetRequest_MaxVariables
}

func (m *GetRequest) GetForwarded() bool {
  if m != nil && m.Forwarded != nil {
    return *m.Forwarded
  }
  return Default_GetRequest_Forwarded
}

func (m *GetRequest) GetMaxValues() uint32 {
  if m != nil && m.MaxValues != nil {
    return *m.MaxValues
  }
  return 0
}

type GetResponse struct {
  Success          *bool          `protobuf:"varint,1,req,name=success" json:"success,omitempty"`
  Errormessage     *string        `protobuf:"bytes,2,opt,name=errormessage" json:"errormessage,omitempty"`
  Stream           []*ValueStream `protobuf:"bytes,3,rep,name=stream" json:"stream,omitempty"`
  XXX_unrecognized []byte         `json:"-"`
}

func (m *GetResponse) Reset()         { *m = GetResponse{} }
func (m *GetResponse) String() string { return proto.CompactTextString(m) }
func (*GetResponse) ProtoMessage()    {}

func (m *GetResponse) GetSuccess() bool {
  if m != nil && m.Success != nil {
    return *m.Success
  }
  return false
}

func (m *GetResponse) GetErrormessage() string {
  if m != nil && m.Errormessage != nil {
    return *m.Errormessage
  }
  return ""
}

func (m *GetResponse) GetStream() []*ValueStream {
  if m != nil {
    return m.Stream
  }
  return nil
}

type AddRequest struct {
  Stream           []*ValueStream `protobuf:"bytes,1,rep,name=stream" json:"stream,omitempty"`
  Forwarded        *bool          `protobuf:"varint,2,opt,name=forwarded,def=0" json:"forwarded,omitempty"`
  XXX_unrecognized []byte         `json:"-"`
}

func (m *AddRequest) Reset()         { *m = AddRequest{} }
func (m *AddRequest) String() string { return proto.CompactTextString(m) }
func (*AddRequest) ProtoMessage()    {}

const Default_AddRequest_Forwarded bool = false

func (m *AddRequest) GetStream() []*ValueStream {
  if m != nil {
    return m.Stream
  }
  return nil
}

func (m *AddRequest) GetForwarded() bool {
  if m != nil && m.Forwarded != nil {
    return *m.Forwarded
  }
  return Default_AddRequest_Forwarded
}

type AddResponse struct {
  Success          *bool   `protobuf:"varint,1,req,name=success" json:"success,omitempty"`
  Errormessage     *string `protobuf:"bytes,2,opt,name=errormessage" json:"errormessage,omitempty"`
  XXX_unrecognized []byte  `json:"-"`
}

func (m *AddResponse) Reset()         { *m = AddResponse{} }
func (m *AddResponse) String() string { return proto.CompactTextString(m) }
func (*AddResponse) ProtoMessage()    {}

func (m *AddResponse) GetSuccess() bool {
  if m != nil && m.Success != nil {
    return *m.Success
  }
  return false
}

func (m *AddResponse) GetErrormessage() string {
  if m != nil && m.Errormessage != nil {
    return *m.Errormessage
  }
  return ""
}

type ListRequest struct {
  DEPRECATEDStringPrefix *string         `protobuf:"bytes,1,opt,name=DEPRECATED_string_prefix" json:"DEPRECATED_string_prefix,omitempty"`
  Prefix                 *StreamVariable `protobuf:"bytes,3,opt,name=prefix" json:"prefix,omitempty"`
  MaxVariables           *uint32         `protobuf:"varint,2,opt,name=max_variables,def=100" json:"max_variables,omitempty"`
  XXX_unrecognized       []byte          `json:"-"`
}

func (m *ListRequest) Reset()         { *m = ListRequest{} }
func (m *ListRequest) String() string { return proto.CompactTextString(m) }
func (*ListRequest) ProtoMessage()    {}

const Default_ListRequest_MaxVariables uint32 = 100

func (m *ListRequest) GetDEPRECATEDStringPrefix() string {
  if m != nil && m.DEPRECATEDStringPrefix != nil {
    return *m.DEPRECATEDStringPrefix
  }
  return ""
}

func (m *ListRequest) GetPrefix() *StreamVariable {
  if m != nil {
    return m.Prefix
  }
  return nil
}

func (m *ListRequest) GetMaxVariables() uint32 {
  if m != nil && m.MaxVariables != nil {
    return *m.MaxVariables
  }
  return Default_ListRequest_MaxVariables
}

type ListResponse struct {
  Success          *bool          `protobuf:"varint,1,req,name=success" json:"success,omitempty"`
  Errormessage     *string        `protobuf:"bytes,2,opt,name=errormessage" json:"errormessage,omitempty"`
  Stream           []*ValueStream `protobuf:"bytes,3,rep,name=stream" json:"stream,omitempty"`
  XXX_unrecognized []byte         `json:"-"`
}

func (m *ListResponse) Reset()         { *m = ListResponse{} }
func (m *ListResponse) String() string { return proto.CompactTextString(m) }
func (*ListResponse) ProtoMessage()    {}

func (m *ListResponse) GetSuccess() bool {
  if m != nil && m.Success != nil {
    return *m.Success
  }
  return false
}

func (m *ListResponse) GetErrormessage() string {
  if m != nil && m.Errormessage != nil {
    return *m.Errormessage
  }
  return ""
}

func (m *ListResponse) GetStream() []*ValueStream {
  if m != nil {
    return m.Stream
  }
  return nil
}

type StoreFileHeaderIndex struct {
  Variable         *StreamVariable `protobuf:"bytes,1,req,name=variable" json:"variable,omitempty"`
  Offset           *uint64         `protobuf:"fixed64,2,req,name=offset" json:"offset,omitempty"`
  XXX_unrecognized []byte          `json:"-"`
}

func (m *StoreFileHeaderIndex) Reset()         { *m = StoreFileHeaderIndex{} }
func (m *StoreFileHeaderIndex) String() string { return proto.CompactTextString(m) }
func (*StoreFileHeaderIndex) ProtoMessage()    {}

func (m *StoreFileHeaderIndex) GetVariable() *StreamVariable {
  if m != nil {
    return m.Variable
  }
  return nil
}

func (m *StoreFileHeaderIndex) GetOffset() uint64 {
  if m != nil && m.Offset != nil {
    return *m.Offset
  }
  return 0
}

type StoreFileHeader struct {
  StartTimestamp           *uint64                 `protobuf:"varint,1,req,name=start_timestamp" json:"start_timestamp,omitempty"`
  EndTimestamp             *uint64                 `protobuf:"varint,2,req,name=end_timestamp" json:"end_timestamp,omitempty"`
  DEPRECATEDStringVariable []string                `protobuf:"bytes,3,rep,name=DEPRECATED_string_variable" json:"DEPRECATED_string_variable,omitempty"`
  Variable                 []*StreamVariable       `protobuf:"bytes,4,rep,name=variable" json:"variable,omitempty"`
  Index                    []*StoreFileHeaderIndex `protobuf:"bytes,5,rep,name=index" json:"index,omitempty"`
  XXX_unrecognized         []byte                  `json:"-"`
}

func (m *StoreFileHeader) Reset()         { *m = StoreFileHeader{} }
func (m *StoreFileHeader) String() string { return proto.CompactTextString(m) }
func (*StoreFileHeader) ProtoMessage()    {}

func (m *StoreFileHeader) GetStartTimestamp() uint64 {
  if m != nil && m.StartTimestamp != nil {
    return *m.StartTimestamp
  }
  return 0
}

func (m *StoreFileHeader) GetEndTimestamp() uint64 {
  if m != nil && m.EndTimestamp != nil {
    return *m.EndTimestamp
  }
  return 0
}

func (m *StoreFileHeader) GetDEPRECATEDStringVariable() []string {
  if m != nil {
    return m.DEPRECATEDStringVariable
  }
  return nil
}

func (m *StoreFileHeader) GetVariable() []*StreamVariable {
  if m != nil {
    return m.Variable
  }
  return nil
}

func (m *StoreFileHeader) GetIndex() []*StoreFileHeaderIndex {
  if m != nil {
    return m.Index
  }
  return nil
}

type RetentionPolicyItem struct {
  Variable         []*StreamVariable           `protobuf:"bytes,1,rep,name=variable" json:"variable,omitempty"`
  Comment          []string                    `protobuf:"bytes,2,rep,name=comment" json:"comment,omitempty"`
  Policy           *RetentionPolicyItem_Target `protobuf:"varint,3,req,name=policy,enum=openinstrument.proto.RetentionPolicyItem_Target" json:"policy,omitempty"`
  Mutation         []*StreamMutation           `protobuf:"bytes,4,rep,name=mutation" json:"mutation,omitempty"`
  MinAge           *uint64                     `protobuf:"varint,5,opt,name=min_age,def=0" json:"min_age,omitempty"`
  MaxAge           *uint64                     `protobuf:"varint,6,opt,name=max_age,def=0" json:"max_age,omitempty"`
  XXX_unrecognized []byte                      `json:"-"`
}

func (m *RetentionPolicyItem) Reset()         { *m = RetentionPolicyItem{} }
func (m *RetentionPolicyItem) String() string { return proto.CompactTextString(m) }
func (*RetentionPolicyItem) ProtoMessage()    {}

const Default_RetentionPolicyItem_MinAge uint64 = 0
const Default_RetentionPolicyItem_MaxAge uint64 = 0

func (m *RetentionPolicyItem) GetVariable() []*StreamVariable {
  if m != nil {
    return m.Variable
  }
  return nil
}

func (m *RetentionPolicyItem) GetComment() []string {
  if m != nil {
    return m.Comment
  }
  return nil
}

func (m *RetentionPolicyItem) GetPolicy() RetentionPolicyItem_Target {
  if m != nil && m.Policy != nil {
    return *m.Policy
  }
  return 0
}

func (m *RetentionPolicyItem) GetMutation() []*StreamMutation {
  if m != nil {
    return m.Mutation
  }
  return nil
}

func (m *RetentionPolicyItem) GetMinAge() uint64 {
  if m != nil && m.MinAge != nil {
    return *m.MinAge
  }
  return Default_RetentionPolicyItem_MinAge
}

func (m *RetentionPolicyItem) GetMaxAge() uint64 {
  if m != nil && m.MaxAge != nil {
    return *m.MaxAge
  }
  return Default_RetentionPolicyItem_MaxAge
}

type RetentionPolicy struct {
  Policy           []*RetentionPolicyItem `protobuf:"bytes,1,rep,name=policy" json:"policy,omitempty"`
  Interval         *uint32                `protobuf:"varint,2,opt,name=interval,def=600" json:"interval,omitempty"`
  XXX_unrecognized []byte                 `json:"-"`
}

func (m *RetentionPolicy) Reset()         { *m = RetentionPolicy{} }
func (m *RetentionPolicy) String() string { return proto.CompactTextString(m) }
func (*RetentionPolicy) ProtoMessage()    {}

const Default_RetentionPolicy_Interval uint32 = 600

func (m *RetentionPolicy) GetPolicy() []*RetentionPolicyItem {
  if m != nil {
    return m.Policy
  }
  return nil
}

func (m *RetentionPolicy) GetInterval() uint32 {
  if m != nil && m.Interval != nil {
    return *m.Interval
  }
  return Default_RetentionPolicy_Interval
}

type StoreServer struct {
  Address          *string            `protobuf:"bytes,1,req,name=address" json:"address,omitempty"`
  State            *StoreServer_State `protobuf:"varint,2,opt,name=state,enum=openinstrument.proto.StoreServer_State" json:"state,omitempty"`
  LastUpdated      *uint64            `protobuf:"varint,3,opt,name=last_updated" json:"last_updated,omitempty"`
  XXX_unrecognized []byte             `json:"-"`
}

func (m *StoreServer) Reset()         { *m = StoreServer{} }
func (m *StoreServer) String() string { return proto.CompactTextString(m) }
func (*StoreServer) ProtoMessage()    {}

func (m *StoreServer) GetAddress() string {
  if m != nil && m.Address != nil {
    return *m.Address
  }
  return ""
}

func (m *StoreServer) GetState() StoreServer_State {
  if m != nil && m.State != nil {
    return *m.State
  }
  return 0
}

func (m *StoreServer) GetLastUpdated() uint64 {
  if m != nil && m.LastUpdated != nil {
    return *m.LastUpdated
  }
  return 0
}

type StoreConfig struct {
  Server           []*StoreServer   `protobuf:"bytes,1,rep,name=server" json:"server,omitempty"`
  RetentionPolicy  *RetentionPolicy `protobuf:"bytes,2,opt,name=retention_policy" json:"retention_policy,omitempty"`
  XXX_unrecognized []byte           `json:"-"`
}

func (m *StoreConfig) Reset()         { *m = StoreConfig{} }
func (m *StoreConfig) String() string { return proto.CompactTextString(m) }
func (*StoreConfig) ProtoMessage()    {}

func (m *StoreConfig) GetServer() []*StoreServer {
  if m != nil {
    return m.Server
  }
  return nil
}

func (m *StoreConfig) GetRetentionPolicy() *RetentionPolicy {
  if m != nil {
    return m.RetentionPolicy
  }
  return nil
}

func init() {
  proto.RegisterEnum("openinstrument.proto.StreamVariable_ValueType", StreamVariable_ValueType_name, StreamVariable_ValueType_value)
  proto.RegisterEnum("openinstrument.proto.StreamMutation_SampleType", StreamMutation_SampleType_name, StreamMutation_SampleType_value)
  proto.RegisterEnum("openinstrument.proto.StreamAggregation_AggregateType", StreamAggregation_AggregateType_name, StreamAggregation_AggregateType_value)
  proto.RegisterEnum("openinstrument.proto.RetentionPolicyItem_Target", RetentionPolicyItem_Target_name, RetentionPolicyItem_Target_value)
  proto.RegisterEnum("openinstrument.proto.StoreServer_State", StoreServer_State_name, StoreServer_State_value)
}
