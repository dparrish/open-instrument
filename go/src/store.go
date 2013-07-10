package main

import (
  "code.google.com/p/goprotobuf/proto"
  "code.google.com/p/open-instrument"
  "code.google.com/p/open-instrument/store_manager"
  "code.google.com/p/open-instrument/variable"
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
  //"bytes"
  //"strings"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
)

var address = flag.String("address", "", "Address to listen on (blank for any)")
var port = flag.Int("port", 8020, "Port to listen on")

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
  stream_chan := make(chan *openinstrument_proto.ValueStream)
  go smanager.GetValueStreams(request_variable, request.MinTimestamp, request.MaxTimestamp, stream_chan)
  response.Stream = make([]*openinstrument_proto.ValueStream, 0)
  var count uint32
  for {
    stream := <-stream_chan
    if stream == nil {
      break
    }
    newstream := new(openinstrument_proto.ValueStream)
    newstream.Variable = variable.NewFromProto(stream.Variable).AsProto()
    var value_count uint32
    for _, value := range stream.Value {
      if request.MinTimestamp != nil && value.GetTimestamp() < request.GetMinTimestamp() {
        // Too old
        continue
      }
      if request.MaxTimestamp != nil && value.GetTimestamp() > request.GetMaxTimestamp() {
        // Too new
        continue
      }
      newstream.Value = append(newstream.Value, value)
      value_count++
    }
    if request.MaxValues != nil && value_count >= request.GetMaxValues() {
      newstream.Value = newstream.Value[uint32(len(newstream.Value)) - request.GetMaxValues():]
    }
    response.Stream = append(response.Stream, newstream)
    count++
    if count >= request.GetMaxVariables() {
      break
    }
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

  stream_chan := make(chan *openinstrument_proto.ValueStream)
  go smanager.AddValueStreams(stream_chan)
  for _, stream := range request.Stream {
    stream_chan <- stream
  }
  stream_chan <- nil

  response.Success = proto.Bool(true)
  returnResponse(w, req, &response)
}

func List(w http.ResponseWriter, req *http.Request) {
  var request openinstrument_proto.ListRequest
  var response openinstrument_proto.ListResponse
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
  stream_chan := make(chan *openinstrument_proto.ValueStream)
  go smanager.GetValueStreams(request_variable, nil, nil, stream_chan)
  response.Stream = make([]*openinstrument_proto.ValueStream, 0)
  var count uint32
  for {
    stream := <-stream_chan
    if stream == nil {
      break
    }
    newstream := openinstrument_proto.ValueStream{
      Variable: stream.Variable,
    }
    response.Stream = append(response.Stream, &newstream)
    count++
    if count >= request.GetMaxVariables() {
      break
    }
  }

  response.Success = proto.Bool(true)
  returnResponse(w, req, &response)
}

// Argument server.
func Args(w http.ResponseWriter, req *http.Request) {
  fmt.Fprintln(w, os.Args[1:])
}

func main() {
  flag.Parse()
  http.Handle("/list", http.HandlerFunc(List))
  http.Handle("/get", http.HandlerFunc(Get))
  http.Handle("/add", http.HandlerFunc(Add))
  http.Handle("/args", http.HandlerFunc(Args))
  log.Printf("Current PID: %d", os.Getpid())
  sock, e := net.ListenTCP("tcp", &net.TCPAddr{net.ParseIP(*address), *port})
  if e != nil {
    log.Fatal("Can't listen on %s: %s", net.JoinHostPort(*address, strconv.Itoa(*port)), e)
  }
  log.Printf("Listening on %v", sock.Addr().String())
  go smanager.Run()
  http.Serve(sock, nil)
}
