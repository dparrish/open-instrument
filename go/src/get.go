package main

import (
  "code.google.com/p/goprotobuf/proto"
  "code.google.com/p/open-instrument"
  "flag"
  "fmt"
  "log"
  "net"
  "net/http"
  "os"
  "strconv"
  "time"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
)

var address = flag.String("address", "", "Address to listen on (blank for any)")
var port = flag.Int("port", 0, "Port to listen on")
var max_variables = flag.Int("max_variables", 0, "Maximum number of variables to return")
var max_values = flag.Int("max_values", 0, "Maximum number of values to return for each variable. This returns the latest matching values.")
var duration = flag.String("duration", "12h", "Duration of data to request")

func List(w http.ResponseWriter, req *http.Request) {
  fmt.Fprintf(w, "Hello")
}

// Argument server.
func Args(w http.ResponseWriter, req *http.Request) {
  fmt.Fprintln(w, os.Args)
}

func StartServer() {
  http.Handle("/list", http.HandlerFunc(List))
  http.Handle("/args", http.HandlerFunc(Args))
  log.Printf("Current PID: %d", os.Getpid())
  sock, e := net.ListenTCP("tcp", &net.TCPAddr{net.ParseIP(*address), *port})
  if e != nil {
    log.Fatal("Can't listen on %s: %s", net.JoinHostPort(*address, strconv.Itoa(*port)), e)
  }
  log.Printf("Listening on %v", sock.Addr().String())
  go http.Serve(sock, nil)
}

func main() {
  flag.Parse()

  host, portstr, _ := net.SplitHostPort(flag.Arg(0))
  port, _ := strconv.Atoi(portstr)
  client := openinstrument.NewStoreClient(host, port)
  dur, err := time.ParseDuration(*duration)
  if err != nil {
    log.Fatal("Invalid --duration:", err)
  }
  request := openinstrument_proto.GetRequest{
    Variable: openinstrument.NewVariableFromString(flag.Arg(1)).AsProto(),
    MinTimestamp: proto.Uint64(uint64(time.Now().Add(-dur).UnixNano() / 1000000)),
  }
  if *max_variables > 0 {
    request.MaxVariables = proto.Uint32(uint32(*max_variables))
  }
  if *max_values > 0 {
    request.MaxValues = proto.Uint32(uint32(*max_values))
  }
  getresponse, err := client.Get(&request)
  if err != nil {
    log.Fatal(err)
  }
  for _, stream := range getresponse.Stream {
    variable := openinstrument.NewVariableFromProto(stream.Variable).String()
    for _, value := range stream.Value {
      fmt.Printf("%s\t%s\t", variable, time.Unix(int64(*value.Timestamp / 1000), 0))
      if value.DoubleValue != nil {
        fmt.Printf("%f\n", *value.DoubleValue)
      } else if value.StringValue != nil {
        fmt.Printf("%s\n", value.StringValue)
      }
    }
  }
}

