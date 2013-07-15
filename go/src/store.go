package main

import (
  "code.google.com/p/goprotobuf/proto"
  "code.google.com/p/open-instrument"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
  "code.google.com/p/open-instrument/store_config"
  "code.google.com/p/open-instrument/store_manager"
  "code.google.com/p/open-instrument/variable"
  "code.google.com/p/open-instrument/mutations"
  "encoding/base64"
  "errors"
  "flag"
  "fmt"
  "io/ioutil"
  "log"
  "net"
  "net/http"
  "os"
  "strconv"
  "time"
)

var address = flag.String("address", "", "Address to listen on (blank for any)")
var port = flag.Int("port", 8020, "Port to listen on")
var config_file = flag.String("config", "/r2/services/openinstrument/config.txt",
  "Path to the store configuration file. If the format is host:port:/path, then ZooKeeper will be used to access it.")

var smanager store_manager.StoreManager

func parseRequest(w http.ResponseWriter, req *http.Request, request proto.Message) error {
  body, _ := ioutil.ReadAll(req.Body)
  defer req.Body.Close()
  decoded_body, err := base64.StdEncoding.DecodeString(string(body))
  if err != nil {
    w.WriteHeader(http.StatusBadRequest)
    fmt.Fprintf(w, "Invalid body: %s", err)
    return errors.New("Invalid body")
  }
  if err = proto.Unmarshal(decoded_body, request); err != nil {
    w.WriteHeader(http.StatusBadRequest)
    fmt.Fprintf(w, "Invalid GetRequest: %s", err)
    return errors.New("Invalid GetRequest")
  }
  //log.Printf("Request: %s", request)
  return nil
}

func returnResponse(w http.ResponseWriter, req *http.Request, response proto.Message) error {
  //log.Printf("Response: %s", response)
  data, err := proto.Marshal(response)
  if err != nil {
    w.WriteHeader(http.StatusInternalServerError)
    fmt.Fprintf(w, "Error encoding response: %s", err)
    return errors.New("Error encoding response")
  }
  //encoded_data := base64.StdEncoding.EncodeToString(data)
  //w.Write([]byte(encoded_data))
  encoder := base64.NewEncoder(base64.StdEncoding, w)
  encoder.Write(data)
  encoder.Close()
  return nil
}

func Get(w http.ResponseWriter, req *http.Request) {
  var request openinstrument_proto.GetRequest
  var response openinstrument_proto.GetResponse
  if parseRequest(w, req, &request) != nil {
    return
  }
  if request.GetVariable() == nil {
    w.WriteHeader(http.StatusBadRequest)
    response.Success = proto.Bool(false)
    response.Errormessage = proto.String("No variable specified")
    returnResponse(w, req, &response)
    return
  }
  request_variable := variable.NewFromProto(request.Variable)
  if len(request_variable.Variable) == 0 {
    w.WriteHeader(http.StatusBadRequest)
    response.Success = proto.Bool(false)
    response.Errormessage = proto.String("No variable specified")
    returnResponse(w, req, &response)
    return
  }
  fmt.Println(openinstrument.ProtoText(&request))
  stream_chan := smanager.GetValueStreams(request_variable, request.MinTimestamp, request.MaxTimestamp)
  streams := make([]*openinstrument_proto.ValueStream, 0)
  for stream := range stream_chan {
    streams = append(streams, stream)
  }
  merge_by := ""
  if len(request.Aggregation) > 0 {
    merge_by = request.Aggregation[0].GetLabel()[0]
  }
  sc := openinstrument.MergeStreamsBy(streams, merge_by)
  response.Stream = make([]*openinstrument_proto.ValueStream, 0)
  for streams := range sc {
    mutation_channels := openinstrument.ValueStreamChannelList(openinstrument.MergeValueStreams(streams))
    if request.GetMutation() != nil && len(request.GetMutation()) > 0 {
      for _, mut := range request.GetMutation() {
        switch mut.GetSampleType() {
        case openinstrument_proto.StreamMutation_NONE:
          mutation_channels.Add(mutations.MutateValues(uint64(mut.GetSampleFrequency()),
                                                       mutation_channels.Last(),
                                                       mutations.Interpolate))
        case openinstrument_proto.StreamMutation_AVERAGE:
          mutation_channels.Add(mutations.MutateValues(uint64(mut.GetSampleFrequency()),
                                                       mutation_channels.Last(),
                                                       mutations.Mean))
        case openinstrument_proto.StreamMutation_MIN:
          mutation_channels.Add(mutations.MutateValues(uint64(mut.GetSampleFrequency()),
                                                       mutation_channels.Last(),
                                                       mutations.Min))
        case openinstrument_proto.StreamMutation_MAX:
          mutation_channels.Add(mutations.MutateValues(uint64(mut.GetSampleFrequency()),
                                                       mutation_channels.Last(),
                                                       mutations.Max))
        case openinstrument_proto.StreamMutation_RATE:
          mutation_channels.Add(mutations.MutateValues(uint64(mut.GetSampleFrequency()),
                                                       mutation_channels.Last(),
                                                       mutations.Rate))
        case openinstrument_proto.StreamMutation_RATE_SIGNED:
          mutation_channels.Add(mutations.MutateValues(uint64(mut.GetSampleFrequency()),
                                                       mutation_channels.Last(),
                                                       mutations.SignedRate))
        }
      }
    }

    newstream := new(openinstrument_proto.ValueStream)
    newstream.Variable = variable.NewFromProto(streams[0].Variable).AsProto()
    writer := openinstrument.ValueStreamWriter(newstream)
    var value_count uint32
    for value := range mutation_channels.Last() {
      if request.MinTimestamp != nil && value.GetTimestamp() < request.GetMinTimestamp() {
        // Too old
        continue
      }
      if request.MaxTimestamp != nil && value.GetTimestamp() > request.GetMaxTimestamp() {
        // Too new
        continue
      }
      writer <- value
      value_count++
    }
    close(writer)

    if request.MaxValues != nil && value_count >= request.GetMaxValues() {
      newstream.Value = newstream.Value[uint32(len(newstream.Value))-request.GetMaxValues():]
    }

    response.Stream = append(response.Stream, newstream)
  }
  response.Success = proto.Bool(true)
  returnResponse(w, req, &response)
}

func Add(w http.ResponseWriter, req *http.Request) {
  var request openinstrument_proto.AddRequest
  var response openinstrument_proto.AddResponse
  if parseRequest(w, req, &request) != nil {
    return
  }

  stream_chan := smanager.AddValueStreams()
  for _, stream := range request.Stream {
    stream_chan <- stream
  }
  close(stream_chan)

  response.Success = proto.Bool(true)
  returnResponse(w, req, &response)
}

func ListResponseAddTimer(name string, response *openinstrument_proto.ListResponse) *openinstrument.Timer {
  response.Timer = append(response.Timer, &openinstrument_proto.Timer{})
  return openinstrument.NewTimer(name, response.Timer[len(response.Timer)-1])
}

func List(w http.ResponseWriter, req *http.Request) {
  var request openinstrument_proto.ListRequest
  var response openinstrument_proto.ListResponse
  response.Timer = make([]*openinstrument_proto.Timer, 0)
  if parseRequest(w, req, &request) != nil {
    return
  }
  fmt.Println(openinstrument.ProtoText(&request))

  request_variable := variable.NewFromProto(request.Prefix)
  if len(request_variable.Variable) == 0 {
    w.WriteHeader(http.StatusBadRequest)
    response.Success = proto.Bool(false)
    response.Errormessage = proto.String("No variable specified")
    returnResponse(w, req, &response)
    return
  }

  // Retrieve all variables and store the names in a map for uniqueness
  timer := ListResponseAddTimer("retrieve variables", &response)
  vars := make(map[string]*openinstrument_proto.StreamVariable)
  min_timestamp := time.Now().Add(time.Duration(-request.GetMaxAge()) * time.Millisecond)
  unix := uint64(min_timestamp.Unix()) * 1000
  stream_chan := smanager.GetValueStreams(request_variable, &unix, nil)
  for stream := range stream_chan {
    vars[variable.NewFromProto(stream.Variable).String()] = stream.Variable
    if request.GetMaxVariables() > 0 && len(vars) == int(request.GetMaxVariables()) {
      break
    }
  }
  timer.Stop()

  // Build the response out of the map
  timer = ListResponseAddTimer("construct response", &response)
  response.Variable = make([]*openinstrument_proto.StreamVariable, 0)
  for varname := range vars {
    response.Variable = append(response.Variable, variable.NewFromString(varname).AsProto())
  }
  response.Success = proto.Bool(true)
  timer.Stop()
  returnResponse(w, req, &response)
  log.Printf("Timers: %s", response.Timer)
}

// Argument server.
func Args(w http.ResponseWriter, req *http.Request) {
  fmt.Fprintln(w, os.Args[1:])
}

func main() {
  log.SetFlags(log.Ldate | log.Ltime | log.Lshortfile)
  log.Printf("Current PID: %d", os.Getpid())
  flag.Parse()
  config, err := store_config.NewConfig(*config_file)
  if err != nil {
    log.Fatal(err)
  }
  config = config

  http.Handle("/list", http.HandlerFunc(List))
  http.Handle("/get", http.HandlerFunc(Get))
  http.Handle("/add", http.HandlerFunc(Add))
  http.Handle("/args", http.HandlerFunc(Args))
  sock, e := net.ListenTCP("tcp", &net.TCPAddr{net.ParseIP(*address), *port})
  if e != nil {
    log.Fatal("Can't listen on %s: %s", net.JoinHostPort(*address, strconv.Itoa(*port)), e)
  }
  log.Printf("Listening on %v", sock.Addr().String())
  go smanager.Run()
  http.Serve(sock, nil)
}
