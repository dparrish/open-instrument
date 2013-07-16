package store_config

import (
  "code.google.com/p/goprotobuf/proto"
  "errors"
  "fmt"
  "log"
  "os"
  "strings"
  "regexp"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
  "time"
  "net"
)

type config struct {
  filename    string
  config_text string
  Config      *openinstrument_proto.StoreConfig
  stat        os.FileInfo
}

var global_config *config

func NewConfig(filename string) (*config, error) {
  global_config = new(config)
  global_config.filename = filename
  if err := global_config.ReadConfig(); err != nil {
    return nil, err
  }
  go global_config.watchConfigFile()
  return global_config, nil
}

func Config() *config {
  if global_config == nil {
    log.Panic("global config not loaded")
  }
  return global_config
}

func (this *config) ReadConfig() error {
  if this.isZookeeperFile() {
    log.Printf("Zookeeper filename")
    /*
       re, _ := regexp.Compile("^([^:]+:\\d+):(.+)$")
       matches := re.FindStringSubmatch(this.filename)
       zk, session, err := gozk.Init(matches[1], 5000)
       if err != nil {
         return errors.New(fmt.Sprintf("Couldn't connect to ZooKeeper: ", err))
       }
       defer zk.Close()
       event := <-session
       if event.State != gozk.STATE_CONNECTED {
         return errors.New(fmt.Sprintf("Couldn't connect to ZooKeeper"))
       }

       data, _, err := zk.Get(matches[2])
       if err != nil {
         return errors.New(fmt.Sprintf("Couldn't read %s from ZooKeeper: %s", matches[2], err))
       }
       if err := this.handleNewConfig(data); err != nil {
         return err
       }
    */
  } else {
    file, err := os.Open(this.filename)
    if err != nil {
      return errors.New(fmt.Sprintf("Error opening config file %s: %s", this.filename, err))
    }
    defer file.Close()
    this.stat, _ = file.Stat()
    buf := make([]byte, this.stat.Size())
    n, err := file.Read(buf)
    if err != nil || int64(n) != this.stat.Size() {
      return errors.New(fmt.Sprintf("Error reading config file %s: %s", this.filename, err))
    }
    if err := this.handleNewConfig(string(buf)); err != nil {
      return err
    }
  }
  return nil
}

func (this *config) isZookeeperFile() bool {
  re, _ := regexp.Compile("^([^:]+):(\\d+):(.+)$")
  matches := re.FindStringSubmatch(this.filename)
  if len(matches) > 0 && matches[0] != "" {
    return true
  }
  return false
}

func (this *config) handleNewConfig(text string) error {
  config := new(openinstrument_proto.StoreConfig)
  err := proto.UnmarshalText(text, config)
  if err != nil {
    return errors.New(fmt.Sprintf("Error parsing config file %s: %s", this.filename, err))
  }
  this.Config = config
  return nil
}

func (this *config) watchConfigFile() {
  tick := time.Tick(10 * time.Second)
  for {
    select {
    case <-tick:
      stat, _ := os.Stat(this.filename)
      stat = stat
      this.ReadConfig()
    }
  }
}

func (this *config) ThisServer(port string) *openinstrument_proto.StoreServer {
  addrs, _ := net.InterfaceAddrs()
  for _, server := range this.Config.Server {
    for _, addr := range addrs {
      parts := strings.SplitN(addr.String(), "/", 2)
      if server.GetAddress() == net.JoinHostPort(parts[0], port) {
        return server
      }
    }
  }
  return nil
}

