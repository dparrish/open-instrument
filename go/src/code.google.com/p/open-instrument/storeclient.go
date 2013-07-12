package openinstrument

import (
  "bytes"
  "code.google.com/p/goprotobuf/proto"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
  "code.google.com/p/open-instrument/store_config"
  "encoding/base64"
  "errors"
  "fmt"
  "io/ioutil"
  "log"
  "net/http"
  "strings"
  "sync"
)

func ProtoText(msg proto.Message) string {
  buf := new(bytes.Buffer)
  if err := proto.MarshalText(buf, msg); err != nil {
    return ""
  }
  return buf.String()
}

type StoreClient struct {
  hostport []string
}

func NewStoreClient(config_file string) *StoreClient {
  config, err := store_config.NewConfig(config_file)
  if err != nil {
    log.Fatal(err)
  }
  if len(config.Config.Server) == 0 {
    log.Fatal("Store config does not contain any servers to connect to.")
  }

  client := new(StoreClient)
  client.hostport = make([]string, 0)
  for _, server := range config.Config.GetServer() {
    client.hostport = append(client.hostport, server.GetAddress())
  }
  return client
}

func NewDirectStoreClient(hostport string) *StoreClient {
  client := new(StoreClient)
  client.hostport = append(make([]string, 0), hostport)
  return client
}

func (sc *StoreClient) doRequest(hostport string, path string, request, response proto.Message) error {
  client := http.Client{}

  data, err := proto.Marshal(request)
  if err != nil {
    return errors.New(fmt.Sprintf("Marshaling error: %s", err))
  }
  encoded_body := base64.StdEncoding.EncodeToString(data)
  req, err := http.NewRequest("POST", fmt.Sprintf("http://%s/%s", hostport, path),
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
    return errors.New(fmt.Sprintf("Error decoding response: %s\n'%s'", err, body))
  }

  err = proto.Unmarshal(decoded_body, response)
  if err != nil {
    return errors.New(fmt.Sprintf("Unmarshaling error: %s", err))
  }
  return nil
}

func (sc *StoreClient) SimpleList(prefix string) ([]*openinstrument_proto.ListResponse, error) {
  request := &openinstrument_proto.ListRequest{
    Prefix: &openinstrument_proto.StreamVariable{
      Name: proto.String(prefix),
    },
    MaxVariables: proto.Uint32(10),
  }
  return sc.List(request)
}

func (sc *StoreClient) List(request *openinstrument_proto.ListRequest) ([]*openinstrument_proto.ListResponse, error) {
  c := make(chan *openinstrument_proto.ListResponse, len(sc.hostport))
  go func() {
    waitgroup := new(sync.WaitGroup)
    for _, hostport := range sc.hostport {
      waitgroup.Add(1)
      go func() {
        defer waitgroup.Done()
        response := new(openinstrument_proto.ListResponse)
        err := sc.doRequest(hostport, "list", request, response)
        if err != nil {
          log.Printf("Error in Get to %s: %s", hostport, err)
          return
        }
        c <- response
      }()
    }
    waitgroup.Wait()
    close(c)
  }()

  response := make([]*openinstrument_proto.ListResponse, 0)
  for item := range c {
    response = append(response, item)
  }
  return response, nil
}

func (sc *StoreClient) SimpleGet(variable string, min_timestamp, max_timestamp uint64) ([]*openinstrument_proto.GetResponse, error) {
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
  return sc.Get(request)
}

func (sc *StoreClient) Get(request *openinstrument_proto.GetRequest) ([]*openinstrument_proto.GetResponse, error) {
  c := make(chan *openinstrument_proto.GetResponse, len(sc.hostport))
  go func() {
    waitgroup := new(sync.WaitGroup)
    for _, hostport := range sc.hostport {
      waitgroup.Add(1)
      go func() {
        defer waitgroup.Done()
        response := new(openinstrument_proto.GetResponse)
        err := sc.doRequest(hostport, "get", request, response)
        if err != nil {
          log.Printf("Error in Get to %s: %s", hostport, err)
          return
        }
        c <- response
      }()
    }
    waitgroup.Wait()
    close(c)
  }()

  response := make([]*openinstrument_proto.GetResponse, 0)
  for item := range c {
    response = append(response, item)
  }
  return response, nil
}
