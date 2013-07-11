package main

import (
  "code.google.com/p/goprotobuf/proto"
  "code.google.com/p/open-instrument"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
  "code.google.com/p/open-instrument/variable"
  "flag"
  "fmt"
  "log"
  "net"
  "net/http"
  "os"
  "strconv"
)

var address = flag.String("address", "", "Address to listen on (blank for any)")
var port = flag.Int("port", 0, "Port to listen on")
var max_variables = flag.Int("max_variables", 0, "Maximum number of variables to return")

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
  request := openinstrument_proto.ListRequest{
    Prefix: variable.NewFromString(flag.Arg(1)).AsProto(),
  }
  if *max_variables > 0 {
    request.MaxVariables = proto.Uint32(uint32(*max_variables))
  }
  listresponse, err := client.List(&request)
  if err != nil {
    log.Fatal(err)
  }
  //fmt.Println(openinstrument.ProtoText(&listresponse))
  for _, stream := range listresponse.Stream {
    //fmt.Println(variable.NewFromProto(stream.Variable))
    fmt.Println(variable.NewFromProto(stream.Variable))
  }
}
