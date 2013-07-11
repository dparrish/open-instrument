package openinstrument

import (
  "bytes"
  "code.google.com/p/goprotobuf/proto"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
  "encoding/base64"
  "errors"
  "fmt"
  "io/ioutil"
  "net/http"
  "strings"
)

func ProtoText(msg proto.Message) string {
  buf := new(bytes.Buffer)
  if err := proto.MarshalText(buf, msg); err != nil {
    return ""
  }
  return buf.String()
}

type storeClient struct {
  host string
  port int
}

func NewStoreClient(host string, port int) *storeClient {
  client := new(storeClient)
  client.host = host
  client.port = port
  return client
}

func (sc storeClient) doRequest(path string, request, response proto.Message) error {
  client := http.Client{}

  data, err := proto.Marshal(request)
  if err != nil {
    return errors.New(fmt.Sprintf("Marshaling error: %s", err))
  }
  encoded_body := base64.StdEncoding.EncodeToString(data)
  req, err := http.NewRequest("POST", fmt.Sprintf("http://%s:%d/%s", sc.host, sc.port, path),
    strings.NewReader(string(encoded_body)))
  if err != nil {
    return errors.New(fmt.Sprintf("Error creating HTTP request: %s", err))
  }
  req.Header.Add("Content-Type", "text/base64")
  req.Header.Add("Content-Length", fmt.Sprintf("%v", len(encoded_body)))
  req.Header.Add("Connection", "close")

  resp, err := client.Do(req)
  if err != nil {
    return errors.New(fmt.Sprintf("HTTP Request error: %s", err))
  }
  //fmt.Println(resp)
  defer resp.Body.Close()

  body, err := ioutil.ReadAll(resp.Body)
  if err != nil {
    return errors.New(fmt.Sprintf("HTTP Request error: %s", err))
  }

  decoded_body, err := base64.StdEncoding.DecodeString(string(body))
  if err != nil {
    return errors.New(fmt.Sprintf("Error decoding response: %s\n%s", err, body))
  }

  err = proto.Unmarshal(decoded_body, response)
  if err != nil {
    return errors.New(fmt.Sprintf("Unmarshaling error: %s", err))
  }
  return nil
}

func (sc storeClient) SimpleList(prefix string) (response openinstrument_proto.ListResponse, err error) {
  request := &openinstrument_proto.ListRequest{
    Prefix: &openinstrument_proto.StreamVariable{
      Name: proto.String(prefix),
    },
    MaxVariables: proto.Uint32(10),
  }
  response, err = sc.List(request)
  return
}

func (sc storeClient) List(request *openinstrument_proto.ListRequest) (response openinstrument_proto.ListResponse, err error) {
  err = sc.doRequest("list", request, &response)
  return
}

func (sc storeClient) SimpleGet(variable string, min_timestamp, max_timestamp uint64) (response openinstrument_proto.GetResponse, err error) {
  reqvar := NewVariableFromString(variable)
  request := &openinstrument_proto.GetRequest{
    Variable: reqvar.AsProto(),
  }
  if min_timestamp > 0 {
    request.MinTimestamp = proto.Uint64(min_timestamp)
  }
  if max_timestamp > 0 {
    request.MaxTimestamp = proto.Uint64(max_timestamp)
  }
  response, err = sc.Get(request)
  return
}

func (sc storeClient) Get(request *openinstrument_proto.GetRequest) (response openinstrument_proto.GetResponse, err error) {
  err = sc.doRequest("get", request, &response)
  return
}
