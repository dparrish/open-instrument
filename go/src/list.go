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
  "sort"
  "strconv"
  "time"
)

var address = flag.String("address", "", "Address to listen on (blank for any)")
var port = flag.Int("port", 0, "Port to listen on")
var max_variables = flag.Int("max_variables", 0, "Maximum number of variables to return")
var max_age = flag.String("max_age", "", "Maximum history to search for old variables")
var config_file = flag.String("config", "/r2/services/openinstrument/config.txt",
  "Path to the store configuration file. If the format is host:port:/path, then ZooKeeper will be used to access it.")
var connect_address = flag.String("connect", "",
  "Connect directly to the specified datastore server. The Store config will be retrieved from this host and used.")

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
  log.SetFlags(log.Ldate | log.Ltime | log.Lshortfile)
  flag.Parse()

  var client *openinstrument.StoreClient
  if *connect_address != "" {
    client = openinstrument.NewDirectStoreClient(*connect_address)
  } else if *config_file != "" {
    client = openinstrument.NewStoreClient(*config_file)
  } else {
    log.Fatal("Specify either --connect or --config")
  }

  request := openinstrument_proto.ListRequest{
    Prefix: variable.NewFromString(flag.Arg(0)).AsProto(),
  }
  if *max_variables > 0 {
    request.MaxVariables = proto.Uint32(uint32(*max_variables))
  }
  if *max_age != "" {
    d, _ := time.ParseDuration(*max_age)
    request.MaxAge = proto.Uint64(uint64(d.Seconds() * 1000))
  }
  response, err := client.List(&request)
  if err != nil {
    log.Fatal(err)
  }
  vars := make([]string, 0)
  for _, listresponse := range response {
    for _, v := range listresponse.Variable {
      vars = append(vars, variable.NewFromProto(v).String())
    }
  }
  sort.Strings(vars)
  for _, variable := range vars {
    fmt.Println(variable)
  }
}
