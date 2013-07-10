package store_manager

import (
  "code.google.com/p/goprotobuf/proto"
  "code.google.com/p/open-instrument"
  "code.google.com/p/open-instrument/variable"
  "flag"
  "regexp"
  "sort"
  "io"
  "log"
  "errors"
  "fmt"
  "os"
  "path/filepath"
  "time"
  "sync"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
)

var datastore_path = flag.String("datastore", "/r2/services/openinstrument", "Path to datastore files")
var recordlog_max_size = flag.Int64("recordlog_max_size", 50, "Maximum size of recordlog in MB")

var PROTO_MAGIC uint16 = 0xDEAD

type StoreManager struct {
  streams map[string] *openinstrument_proto.ValueStream
  streams_mutex sync.RWMutex
  quit chan bool
  record_log_chan chan *openinstrument_proto.ValueStream
  store_files []*IndexedStoreFile
}

func (this *StoreManager) Run() {
  log.Println("Running StoreManager")
  this.quit = make(chan bool)
  this.record_log_chan = make(chan *openinstrument_proto.ValueStream, 1000)

  // Cache the list of datastore filenames
  this.store_files = make([]*IndexedStoreFile, 0)
  dir, err := os.Open(*datastore_path)
  if err != nil {
    log.Printf("Can't open %s for readdir", *datastore_path)
  } else {
    names, err := dir.Readdirnames(0)
    if err != nil {
      log.Printf("Can't read file names in %s", *datastore_path)
    } else {
      for _, name := range names {
        if matched, _ := regexp.MatchString("^datastore\\.\\d+.bin$", name); matched {
          file := IndexedStoreFile{
            Filename: filepath.Join(*datastore_path, name),
          }
          this.store_files = append(this.store_files, &file)
          go func() {
            file.Open()
            file.Close()
          }()
        }
      }
    }
  }

  // Read the current recordlog
  this.readRecordLog(filepath.Join(*datastore_path, "recordlog"), func(stream *openinstrument_proto.ValueStream) {
    this.addValueStreamNoRecord(stream)
  })

  // Start the goroutine that writes the recordlog
  go this.writeRecordLog()
  tick := time.Tick(10 * time.Second)

  // Wait forever...
  for {
    select {
    case <-this.quit:
      log.Println("StoreManager quitting")
    case <-tick:
      // Keep the 20 most recently used datastore files open
      last_use := func(p1, p2 *IndexedStoreFile) bool {
        return p1.last_use.Unix() < p2.last_use.Unix()
      }
      By(last_use).Sort(this.store_files)
      for i, storefile := range this.store_files {
        if i < len(this.store_files) - 20 {
          storefile.Close()
        }
      }
      continue
    }
  }
  log.Println("Shutting down StoreManager")
}

func reopenRecordLog(filename string) (*os.File, error) {
  file, err := os.OpenFile(filename, os.O_WRONLY | os.O_APPEND | os.O_CREATE, 0664)
  if err != nil || file == nil {
    return nil, errors.New(fmt.Sprintf("Error opening recordlog file: %s", err))
  }
  return file, err
}

// indexRecordLog reads in the recordlog file, runlength-encode the ValueStreams, then write out a new datastore file
// with a header identifying where each record lives on disk.
func (this *StoreManager) indexRecordLog(input_filename string, waitgroup *sync.WaitGroup) error {
  defer waitgroup.Done()
  log.Printf("Indexing %s", input_filename)
  streams := make(map[string] *openinstrument_proto.ValueStream, 0)
  var max_timestamp, min_timestamp uint64
  var input_value_count, output_value_count uint64
  this.readRecordLog(input_filename,
    func(new_stream *openinstrument_proto.ValueStream) {
      new_variable := variable.NewFromProto(new_stream.Variable)
      stream, ok := streams[new_variable.String()]
      if !ok {
        stream = new(openinstrument_proto.ValueStream)
        stream.Variable = new_variable.AsProto()
        streams[new_variable.String()] = stream
      }

      for _, value := range new_stream.Value {
        input_value_count++
        if min_timestamp == 0 || value.GetTimestamp() < min_timestamp {
          min_timestamp = value.GetTimestamp()
        }
        if value.GetTimestamp() > max_timestamp {
          max_timestamp = value.GetTimestamp()
        }
        if value.GetEndTimestamp() > max_timestamp {
          max_timestamp = value.GetEndTimestamp()
        }
        // Run-length encoding of the ValueStream
        if len(stream.Value) > 0 {
          last := stream.Value[len(stream.Value) - 1]
          if (last.GetStringValue() != "" && last.GetStringValue() == value.GetStringValue()) ||
          (last.GetDoubleValue() == value.GetDoubleValue()) {
            if value.GetEndTimestamp() > 0 {
              last.EndTimestamp = value.EndTimestamp
            } else {
              last.EndTimestamp = value.Timestamp
            }
          } else {
            stream.Value = append(stream.Value, value)
            output_value_count++
          }
        } else {
          stream.Value = append(stream.Value, value)
          output_value_count++
        }
      }
    })
  log.Printf("Read in recordlog file containing %d streams from %d to %d, compressed %d values down to %d",
  len(streams), min_timestamp, max_timestamp, input_value_count, output_value_count)

  // Build the header with a 0-index for each variable
  header := openinstrument_proto.StoreFileHeader{
    StartTimestamp: proto.Uint64(min_timestamp),
    EndTimestamp: proto.Uint64(min_timestamp),
  }
  for _, varname := range sortedKeys(streams) {
    i := openinstrument_proto.StoreFileHeaderIndex{
      Variable: variable.NewFromString(varname).AsProto(),
      Offset: proto.Uint64(0),
    }
    header.Index = append(header.Index, &i)
  }

  // Write the header
  filename := fmt.Sprintf("%s/datastore.%d.bin", *datastore_path, max_timestamp)
  tmp_filename := filename + ".new"
  log.Printf("Writing to %s", tmp_filename)
  file, err := openinstrument.WriteProtoFile(tmp_filename)
  if err != nil {
    return errors.New(fmt.Sprintf("Error writing new indexed datastore file %s: %s", tmp_filename, err))
  }
  file.Write(&header)

  // Write all the ValueStreams
  for _, stream := range streams {
    varname := variable.NewFromProto(stream.Variable).String()
    for _, index := range header.Index {
      if variable.NewFromProto(index.Variable).String() == varname {
        index.Offset = proto.Uint64(uint64(file.Tell()))
        break
      }
    }
    file.Write(stream)
  }

  // Re-write the header, including the offsets into the file
  file.WriteAt(0, &header)

  // Re-write the header
  log.Printf("Renaming to %s", filename)
  file.Close()
  if err := os.Rename(tmp_filename, filename); err != nil {
    return errors.New(fmt.Sprintf("Error renaming %s to %s: %s", tmp_filename, filename, err))
  }

  log.Printf("Removing %s", input_filename)
  if err := os.Remove(input_filename); err != nil {
    return errors.New(fmt.Sprintf("Error removing old recordlog file %s: %s", input_filename, err))
  }

  // Add the new store file to the list of files we know about
  found := false
  for _, storefile := range this.store_files {
    if storefile.Filename == filename {
      found = true
      break
    }
  }
  if !found {
    this.store_files = append(this.store_files, &IndexedStoreFile{
      Filename: filename,
    })
  }
  return nil
}

func (this *StoreManager) writeRecordLog() {
  // Goroutine that runs constantly, appending entries to the recordlog
  tick := time.Tick(1 * time.Minute)
  //tick := time.Tick(20 * time.Second)
  filename := filepath.Join(*datastore_path, "recordlog")
  file, err := openinstrument.WriteProtoFile(filename)
  if err != nil {
    log.Println(err)
    return
  }
  for {
    if file == nil {
      file, err = openinstrument.WriteProtoFile(filename)
      if err != nil {
        log.Println(err)
        continue
      }
    }
    select {
    case stream := <-this.record_log_chan:
      n, err := file.Write(stream)
      if err != nil || n != 1 {
        log.Println( err)
        file.Close()
        file = nil
      }
    case <-tick:
      // Rotate record log
      stat, err := file.Stat()
      if err != nil {
        log.Printf("Can't stat recordlog:", err)
        continue
      }
      if stat.Size() >= *recordlog_max_size * 1024 * 1024 {
        log.Printf("Recordlog is %d MB (>%d MB), rotating", stat.Size() / 1024 / 1024, *recordlog_max_size)
        new_filename := fmt.Sprintf("%s.%s", filepath.Join(*datastore_path, "recordlog"),
                                    time.Now().UTC().Format(time.RFC3339))
        err := os.Rename(filename, new_filename)
        if err != nil {
          log.Printf("Error renaming %s to %s", filename, new_filename)
        } else if file != nil {
          file.Close()
          file = nil
          this.streams_mutex.Lock()
          this.streams = nil
          this.streams_mutex.Unlock()
        }
      }

      // Index any recently rotated recordlog files
      dir, err := os.Open(*datastore_path)
      if err != nil {
        log.Printf("Can't open %s for readdir", *datastore_path)
      } else {
        names, err := dir.Readdirnames(0)
        if err != nil {
          log.Printf("Can't read file names in %s", *datastore_path)
        } else {
          // Index all the outstanding recordlogs in parallel
          waitgroup := new(sync.WaitGroup)
          for _, name := range names {
            if matched, _ := regexp.MatchString("^recordlog\\..+$", name); matched {
              waitgroup.Add(1)
              go this.indexRecordLog(filepath.Join(*datastore_path, name), waitgroup)
            }
          }
          waitgroup.Wait()
        }
      }
    }
  }
  log.Println("Closing record log")
  if file != nil {
    file.Close()
  }
}

func (this *StoreManager) readRecordLog(filename string, callback func(*openinstrument_proto.ValueStream)) {
  start_time := time.Now()
  file, err := openinstrument.ReadProtoFile(filename)
  if err != nil {
    log.Printf("Error opening proto file: %s", err)
    return
  }
  defer file.Close()
  var stream_count int
  var value_count int
  for {
    stream := new(openinstrument_proto.ValueStream)
    n, err := file.Read(stream)
    if err == io.EOF {
      break
    }
    if err != nil {
      log.Println(err)
      continue
    }
    if n != 1 {
      log.Printf("Read %v records from proto file", n)
      continue
    }
    callback(stream)
    stream_count++
    value_count += len(stream.Value)
  }
  duration := time.Since(start_time)
  log.Printf("Finished reading %d record log streams containing %d values in %v, StoreManager contains %d streams ",
             stream_count, value_count, duration, len(this.streams))
}

func (this *StoreManager) GetValueStreams(v *variable.Variable, min_timestamp, max_timestamp *uint64,
                                          c chan *openinstrument_proto.ValueStream) {
  this.streams_mutex.RLock()
  defer this.streams_mutex.RUnlock()
  if this.streams == nil {
    this.streams = make(map[string] *openinstrument_proto.ValueStream, 0)
  }
  for key, stream := range this.streams {
    k := variable.NewFromString(key)
    if k.Match(v) {
      c <- stream
    }
  }
  // Find indexed store files that have data matching the requested date range
  for _, file := range this.store_files {
    if min_timestamp != nil && *min_timestamp > file.MaxTimestamp {
      continue
    }
    if max_timestamp != nil && *max_timestamp < file.MinTimestamp {
      continue
    }
    for _, stream := range file.GetStreams(v) {
      c <- stream
    }
  }
  c <- nil
}

func (this *StoreManager) AddValueStreams(c chan *openinstrument_proto.ValueStream) {
  for new_stream := range c {
    if new_stream == nil {
      break
    }
    this.addValueStreamNoRecord(new_stream)
    this.record_log_chan <- new_stream
  }
}

func (this *StoreManager) addValueStreamNoRecord(new_stream *openinstrument_proto.ValueStream) {
  this.streams_mutex.RLock()
  defer this.streams_mutex.RUnlock()
  if this.streams == nil {
    this.streams = make(map[string] *openinstrument_proto.ValueStream, 0)
  }
  new_variable := variable.NewFromProto(new_stream.Variable)
  stream, ok := this.streams[new_variable.String()]
  if ok {
    stream.Value = append(stream.Value, new_stream.Value...)
  } else {
    this.streams[new_variable.String()] = new_stream
  }
}

/*
func NanoToMs(nano int64) uint64 {
  return uint64(nano / 1000 / 1000)
}

func NewValue(timestamp uint64, double_value float64) *openinstrument_proto.Value {
  value := new(openinstrument_proto.Value)
  value.Timestamp = proto.Uint64(timestamp)
  value.DoubleValue = proto.Float64(double_value)
  return value
}
*/

func sortedKeys(m map[string] *openinstrument_proto.ValueStream) []string {
  keys := make([]string, len(m))
  i := 0
  for k, _ := range m {
    keys[i] = k
    i++
  }
  sort.Strings(keys)
  return keys
}

