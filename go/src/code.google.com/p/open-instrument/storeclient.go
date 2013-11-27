package openinstrument

// vim:tw=120

import (
  "bytes"
  "code.google.com/p/goprotobuf/proto"
  oproto "code.google.com/p/open-instrument/proto"
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
  servers  []oproto.StoreServer
  hostport []string
}

// NewAutoClient retrieves the config from the supplied single server and uses that config to create a new StoreClient
// that can talk to the entire cluster.
func NewAutoStoreClient(hostport string) (*StoreClient, error) {
  // Create a client with a temporary connection to a single server
  client := NewDirectStoreClient(hostport)
  config := client.GetConfig()
  if config == nil {
    return nil, errors.New(fmt.Sprintf("Error retrieving config from %s", hostport))
  }

  // Use the returned config to create a new client
  client.servers = make([]oproto.StoreServer, 0)
  for _, server := range config.GetServer() {
    client.servers = append(client.servers, *server)
  }
  return client, nil
}

// NewStoreClient uses a config file to create a new StoreClient that can talk to the entire cluster.
func NewStoreClient(config_file string) (*StoreClient, error) {
  config, err := store_config.NewConfig(config_file)
  if err != nil {
    log.Fatal(err)
  }
  if len(config.Config.Server) == 0 {
    return nil, errors.New("Store config does not contain any servers to connect to.")
  }

  client := new(StoreClient)
  client.servers = make([]oproto.StoreServer, 0)
  for _, server := range config.Config.GetServer() {
    client.servers = append(client.servers, *server)
  }
  return client, nil
}

// NewDirectStoreClient creates a StoreClient that will talk to a single server.
func NewDirectStoreClient(hostport string) *StoreClient {
  client := new(StoreClient)
  state := oproto.StoreServer_RUN
  client.servers = append(make([]oproto.StoreServer, 0), oproto.StoreServer{
    Address: &hostport,
    State:   &state,
  })
  return client
}

func (this *StoreClient) doRequest(hostport string, path string, request, response proto.Message) error {
  client := http.Client{}

  var encoded_body string
  if request != nil {
    data, err := proto.Marshal(request)
    if err != nil {
      return errors.New(fmt.Sprintf("Marshaling error: %s", err))
    }
    encoded_body = base64.StdEncoding.EncodeToString(data)
  }
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
  if resp.StatusCode != 200 {
    return errors.New(fmt.Sprintf("HTTP Request to %s \"%s /%s %s\": %s", hostport, req.Method, req.Proto, path,
      resp.Status))
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

func (this *StoreClient) SimpleList(prefix string) ([]*oproto.ListResponse, error) {
  request := &oproto.ListRequest{
    Prefix: &oproto.StreamVariable{
      Name: proto.String(prefix),
    },
    MaxVariables: proto.Uint32(10),
  }
  return this.List(request)
}

func (this *StoreClient) List(request *oproto.ListRequest) ([]*oproto.ListResponse, error) {
  c := make(chan *oproto.ListResponse, len(this.servers))
  waitgroup := new(sync.WaitGroup)
  count := 0
  for _, server := range this.servers {
    switch server.GetState() {
    case oproto.StoreServer_UNKNOWN:
      continue
    case oproto.StoreServer_LOAD:
      continue
    case oproto.StoreServer_DRAIN:
      continue
    case oproto.StoreServer_READONLY:
      continue
    case oproto.StoreServer_SHUTDOWN:
      continue
    }
    waitgroup.Add(1)
    count++
    go func(server oproto.StoreServer) {
      defer waitgroup.Done()
      response := new(oproto.ListResponse)
      err := this.doRequest(server.GetAddress(), "list", request, response)
      if err != nil {
        log.Printf("Error in Get to %s: %s", server.GetAddress(), err)
        return
      }
      c <- response
    }(server)
  }
  if count == 0 {
    close(c)
    return nil, errors.New("No servers available for Get")
  }
  go func() {
    waitgroup.Wait()
    close(c)
  }()

  response := make([]*oproto.ListResponse, 0)
  for item := range c {
    response = append(response, item)
  }
  return response, nil
}

func (this *StoreClient) SimpleGet(variable string, min_timestamp, max_timestamp uint64) ([]*oproto.GetResponse, error) {
  reqvar := NewVariableFromString(variable)
  request := &oproto.GetRequest{
    Variable: reqvar.AsProto(),
  }
  if min_timestamp > 0 {
    request.MinTimestamp = proto.Uint64(min_timestamp)
  }
  if max_timestamp > 0 {
    request.MaxTimestamp = proto.Uint64(max_timestamp)
  }
  return this.Get(request)
}

func (this *StoreClient) Get(request *oproto.GetRequest) ([]*oproto.GetResponse, error) {
  c := make(chan *oproto.GetResponse, len(this.servers))
  waitgroup := new(sync.WaitGroup)
  count := 0
  for _, server := range this.servers {
    switch server.GetState() {
    case oproto.StoreServer_UNKNOWN:
      continue
    case oproto.StoreServer_LOAD:
      continue
    case oproto.StoreServer_DRAIN:
      continue
    case oproto.StoreServer_READONLY:
      continue
    case oproto.StoreServer_SHUTDOWN:
      continue
    }
    waitgroup.Add(1)
    count++
    go func(server oproto.StoreServer) {
      defer waitgroup.Done()
      response := new(oproto.GetResponse)
      err := this.doRequest(server.GetAddress(), "get", request, response)
      if err != nil {
        log.Printf("Error in Get to %s: %s", server.GetAddress(), err)
        return
      }
      c <- response
    }(server)
  }
  if count == 0 {
    close(c)
    return nil, errors.New("No servers available for Get")
  }
  go func() {
    waitgroup.Wait()
    close(c)
  }()

  response := make([]*oproto.GetResponse, 0)
  for item := range c {
    response = append(response, item)
  }
  return response, nil
}

func (this *StoreClient) GetConfig() *oproto.StoreConfig {
  c := make(chan *oproto.StoreConfig, len(this.servers))
  waitgroup := new(sync.WaitGroup)
  count := 0
  for _, server := range this.servers {
    switch server.GetState() {
    case oproto.StoreServer_UNKNOWN:
      continue
    case oproto.StoreServer_LOAD:
      continue
    case oproto.StoreServer_DRAIN:
      continue
    case oproto.StoreServer_READONLY:
      continue
    case oproto.StoreServer_SHUTDOWN:
      continue
    }
    waitgroup.Add(1)
    count++
    go func(server oproto.StoreServer) {
      defer waitgroup.Done()
      request := new(oproto.GetRequest)
      response := new(oproto.StoreConfig)
      err := this.doRequest(server.GetAddress(), "config", request, response)
      if err != nil {
        log.Printf("Error in GetConfig to %s: %s", server.GetAddress(), err)
        return
      }
      c <- response
    }(server)
  }
  if count == 0 {
    close(c)
    log.Printf("No servers available for GetConfig")
    return nil
  }
  go func() {
    waitgroup.Wait()
    close(c)
  }()

  for item := range c {
    // Return the first config block found
    return item
  }
  return nil
}
