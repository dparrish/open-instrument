package main

import (
  "code.google.com/p/goprotobuf/proto"
  "code.google.com/p/open-instrument"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
  "flag"
  "fmt"
  "log"
  "net/http"
  "os"
  "time"
)

var max_variables = flag.Int("max_variables", 0, "Maximum number of variables to return")
var max_values = flag.Int("max_values", 0, "Maximum number of values to return for each variable. This returns the latest matching values.")
var duration = flag.String("duration", "12h", "Duration of data to request")
var config_file = flag.String("config", "/r2/services/openinstrument/config.txt",
  "Path to the store configuration file. If the format is host:port:/path, then ZooKeeper will be used to access it.")
var connect_address = flag.String("connect", "",
  "Connect directly to the specified datastore server. The Store config will be retrieved from this host and used.")
var request_rate = flag.Bool("rate", false, "Return the rate of values over --interval (or original times).")
var request_interval = flag.String("interval", "",
  "Interval between output samples. Provide an empty string to get the raw data.")
var request_mean = flag.Bool("mean", false, "Return the mean of values, over --interval.")
var request_min = flag.Bool("min", false, "Return the minimum value, over --interval.")
var request_max = flag.Bool("max", false, "Return the maximum value, over --interval.")

func List(w http.ResponseWriter, req *http.Request) {
  fmt.Fprintf(w, "Hello")
}

// Argument server.
func Args(w http.ResponseWriter, req *http.Request) {
  fmt.Fprintln(w, os.Args)
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

  // Build the request
  dur, err := time.ParseDuration(*duration)
  if err != nil {
    log.Fatal("Invalid --duration:", err)
  }
  request := openinstrument_proto.GetRequest{
    Variable:     openinstrument.NewVariableFromString(flag.Arg(0)).AsProto(),
    MinTimestamp: proto.Uint64(uint64(time.Now().Add(-dur).UnixNano() / 1000000)),
  }
  if *max_variables > 0 {
    request.MaxVariables = proto.Uint32(uint32(*max_variables))
  }
  if *max_values > 0 {
    request.MaxValues = proto.Uint32(uint32(*max_values))
  }
  request.Mutation = make([]*openinstrument_proto.StreamMutation, 0)
  if *request_rate {
    if *request_interval != "" {
      sample_frequency, err := time.ParseDuration(*request_interval)
      if err != nil {
        log.Fatal("Invalid --interval:", err)
      }
      i := openinstrument_proto.StreamMutation_NONE
      request.Mutation = append(request.Mutation, &openinstrument_proto.StreamMutation{
        SampleType: &i,
        SampleFrequency: proto.Uint32(uint32(sample_frequency.Nanoseconds() / 1000000)),
      })
    }
    i := openinstrument_proto.StreamMutation_RATE
    request.Mutation = append(request.Mutation, &openinstrument_proto.StreamMutation{
      SampleType: &i,
    })
  } else if *request_mean {
    if *request_interval == "" {
      log.Fatal("--interval required")
    }
    sample_frequency, err := time.ParseDuration(*request_interval)
    if err != nil {
      log.Fatal("Invalid --interval:", err)
    }
    i := openinstrument_proto.StreamMutation_AVERAGE
    request.Mutation = append(request.Mutation, &openinstrument_proto.StreamMutation{
      SampleType: &i,
      SampleFrequency: proto.Uint32(uint32(sample_frequency.Nanoseconds() / 1000000)),
    })
  } else if *request_max {
    if *request_interval == "" {
      log.Fatal("--interval required")
    }
    sample_frequency, err := time.ParseDuration(*request_interval)
    if err != nil {
      log.Fatal("Invalid --interval:", err)
    }
    i := openinstrument_proto.StreamMutation_MAX
    request.Mutation = append(request.Mutation, &openinstrument_proto.StreamMutation{
      SampleType: &i,
      SampleFrequency: proto.Uint32(uint32(sample_frequency.Nanoseconds() / 1000000)),
    })
  } else if *request_min {
    if *request_interval == "" {
      log.Fatal("--interval required")
    }
    sample_frequency, err := time.ParseDuration(*request_interval)
    if err != nil {
      log.Fatal("Invalid --interval:", err)
    }
    i := openinstrument_proto.StreamMutation_MIN
    request.Mutation = append(request.Mutation, &openinstrument_proto.StreamMutation{
      SampleType: &i,
      SampleFrequency: proto.Uint32(uint32(sample_frequency.Nanoseconds() / 1000000)),
    })
  } else if *request_interval != "" {
    sample_frequency, err := time.ParseDuration(*request_interval)
    if err != nil {
      log.Fatal("Invalid --interval:", err)
    }
    i := openinstrument_proto.StreamMutation_NONE
    request.Mutation = append(request.Mutation, &openinstrument_proto.StreamMutation{
      SampleType: &i,
      SampleFrequency: proto.Uint32(uint32(sample_frequency.Nanoseconds() / 1000000)),
    })
  }

  response, err := client.Get(&request)
  if err != nil {
    log.Fatal(err)
  }
  for _, getresponse := range response {
    for _, stream := range getresponse.Stream {
      variable := openinstrument.NewVariableFromProto(stream.Variable).String()
      for _, value := range stream.Value {
        fmt.Printf("%s\t%s\t", variable, time.Unix(int64(*value.Timestamp/1000), 0))
        if value.DoubleValue != nil {
          if *request_rate {
            fmt.Printf("%f\n", *value.DoubleValue * 1000.0)
          } else {
            fmt.Printf("%f\n", *value.DoubleValue)
          }
        } else if value.StringValue != nil {
          fmt.Printf("%s\n", value.StringValue)
        }
      }
    }
  }
}
