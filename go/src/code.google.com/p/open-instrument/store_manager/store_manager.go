package store_manager

// vim: tw=120

import (
  "code.google.com/p/goprotobuf/proto"
  "code.google.com/p/open-instrument"
  oproto "code.google.com/p/open-instrument/proto"
  "code.google.com/p/open-instrument/store_config"
  "code.google.com/p/open-instrument/variable"
  "errors"
  "flag"
  "fmt"
  "log"
  "net"
  "os"
  "path/filepath"
  "regexp"
  "sort"
  "sync"
  "time"
)

var datastore_path = flag.String("datastore", "/r2/services/openinstrument", "Path to datastore files")
var recordlog_max_size = flag.Int64("recordlog_max_size", 50, "Maximum size of recordlog in MB")
var datastore_idle_files_open = flag.Int("datastore_idle_files_open", 20,
  "Number of indexed datastore files to keep open at idle")

var PROTO_MAGIC uint16 = 0xDEAD

type StoreManager struct {
  streams         map[string]*oproto.ValueStream
  streams_mutex   sync.RWMutex
  record_log_chan chan *oproto.ValueStream
  store_files     []*IndexedStoreFile
  address         string
  rotating_mutex  sync.Mutex
}

func (this *StoreManager) Run() {
  log.Println("Running StoreManager")
  this.record_log_chan = make(chan *oproto.ValueStream, 1000)

  // Cache the list of datastore filenames
  this.store_files = make([]*IndexedStoreFile, 0)
  names, err := openinstrument.Readdirnames(*datastore_path)
  if err != nil {
    log.Println(err)
  } else {
    start_time := time.Now()
    waitgroup := new(sync.WaitGroup)
    for _, name := range names {
      if matched, _ := regexp.MatchString("^datastore\\.\\d+.bin$", name); matched {
        file := NewIndexedStoreFile(filepath.Join(*datastore_path, name))
        this.store_files = append(this.store_files, file)
        waitgroup.Add(1)
        go func(file *IndexedStoreFile) {
          // Open the file to read the header, then immediately close it to free up filehandles.
          file.Open()
          file.Close()
          waitgroup.Done()
        }(file)
      }
    }
    waitgroup.Wait()
    duration := time.Since(start_time)
    log.Printf("Finished reading headers of datastore files in %s", duration)
  }

  // Read the current recordlog
  duration := this.readRecordLog(filepath.Join(*datastore_path, "recordlog"),
    func(stream *oproto.ValueStream) {
      this.addValueStreamNoRecord(stream)
    })
  log.Printf("Finished reading record log in %v, StoreManager contains %d streams ", duration, len(this.streams))

  // Start the goroutine that writes the recordlog
  go this.writeRecordLog()

  // Wait forever...
  tick := time.Tick(5 * time.Second)
  for {
    <-tick
    this.closeIndexedFiles()
    this.mergeIndexedFiles()
    continue
  }
}

func (this *StoreManager) closeIndexedFiles() {
  // Keep some recently used datastore files open
  last_use := func(p1, p2 *IndexedStoreFile) bool {
    return p1.last_use.Unix() > p2.last_use.Unix()
  }
  By(last_use).Sort(this.store_files)
  var open_count int
  nf := this.numOpenFiles()
  for _, storefile := range this.store_files {
    if !storefile.IsOpen() {
      continue
    }
    if open_count >= *datastore_idle_files_open {
      // Keep indexed files open for 30 seconds after their last use, unless we're close to the limit of open files
      if time.Since(storefile.LastUse()) > time.Duration(30)*time.Second || nf >= 500*0.8 {
        storefile.Close()
        nf--
        continue
      }
    }
    open_count++
  }
}

func (this *StoreManager) mergeIndexedFiles() {
  this.rotating_mutex.Lock()
  defer this.rotating_mutex.Unlock()
  _, port, _ := net.SplitHostPort(this.address)
  config := store_config.Config().ThisServer(port)
  if config == nil {
    log.Printf("mergeIndexedFiles couldn't find server record")
    return
  }
  if config.TargetIndexedFileSize == nil {
    return
  }

  names, err := openinstrument.Readdirnames(*datastore_path)
  if err != nil {
    log.Println(err)
    return
  }

  do_index_files := func(files []string, waitgroup *sync.WaitGroup) {
    defer waitgroup.Done()
    var max_timestamp, min_timestamp uint64
    var value_count, stream_count uint64
    streams := make(map[string]*oproto.ValueStream, 0)
    for _, filename := range files {
      this.readRecordLog(filename,
        func(new_stream *oproto.ValueStream) {
          stream_count++
          new_variable := variable.NewFromProto(new_stream.Variable)
          stream, ok := streams[new_variable.String()]
          if !ok {
            stream = new(oproto.ValueStream)
            stream.Variable = new_variable.AsProto()
            streams[new_variable.String()] = stream
          }

          writer := openinstrument.ValueStreamWriter(stream)
          for _, value := range new_stream.Value {
            value_count++
            if min_timestamp == 0 || value.GetTimestamp() < min_timestamp {
              min_timestamp = value.GetTimestamp()
            }
            if value.GetTimestamp() > max_timestamp {
              max_timestamp = value.GetTimestamp()
            }
            if value.GetEndTimestamp() > max_timestamp {
              max_timestamp = value.GetEndTimestamp()
            }
            writer <- value
          }
          close(writer)
        })
    }
    log.Printf("Read in %d indexed files containing %d streams and %d values", len(files), stream_count, value_count)
    new_filename, err := this.writeIndexedFile(streams, min_timestamp, max_timestamp)
    if err != nil {
      log.Fatal("Error writing indexed file: %s", err)
    }
    for _, filename := range files {
      if filename == new_filename {
        continue
      }
      if err := os.Remove(filename); err != nil {
        log.Fatal("Error removing old recordlog file %s: %s", filename, err)
      }
    }
  }

  var size int64
  files := make([]string, 0)
  waitgroup := new(sync.WaitGroup)
  for _, name := range names {
    if matched, _ := regexp.MatchString("^datastore\\.(\\d+)\\.bin$", name); matched {
      stat, err := os.Stat(filepath.Join(*datastore_path, name))
      if err != nil {
        log.Printf("Can't stat %s: %s", filepath.Join(*datastore_path, name), err)
        continue
      }
      if uint64(size+stat.Size()) > config.GetTargetIndexedFileSize() {
        if len(files) > 1 {
          log.Printf("Combining %d files (%d MB)", len(files), size/1024/1024)
          waitgroup.Add(1)
          do_index_files(files, waitgroup)
        } else {
          // log.Printf("Single file %s is too large to combine", files)
        }
        size = 0
        files = make([]string, 0)
      }
      size += stat.Size()
      files = append(files, filepath.Join(*datastore_path, name))
    }
  }
  if len(files) > 1 {
    log.Printf("At end, combining %d files (%d MB)", len(files), size/1024/1024)
    waitgroup.Add(1)
    do_index_files(files, waitgroup)
  }
  waitgroup.Wait()
}

func reopenRecordLog(filename string) (*os.File, error) {
  file, err := os.OpenFile(filename, os.O_WRONLY|os.O_APPEND|os.O_CREATE, 0664)
  if err != nil {
    return nil, errors.New(fmt.Sprintf("Error opening recordlog file: %s", err))
  }
  return file, err
}

func (this *StoreManager) writeIndexedFile(streams map[string]*oproto.ValueStream, min_timestamp, max_timestamp uint64) (string, error) {
  // Build the header with a 0-index for each variable
  header := oproto.StoreFileHeader{
    StartTimestamp: proto.Uint64(min_timestamp),
    EndTimestamp:   proto.Uint64(min_timestamp),
  }
  for _, varname := range sortedKeys(streams) {
    i := oproto.StoreFileHeaderIndex{
      Variable: variable.NewFromString(varname).AsProto(),
      Offset:   proto.Uint64(0),
    }
    header.Index = append(header.Index, &i)
  }

  // Write the header
  filename := fmt.Sprintf("%s/datastore.%d.bin", *datastore_path, max_timestamp)
  tmp_filename := filename + ".new"
  log.Printf("Writing to %s", tmp_filename)
  file, err := openinstrument.WriteProtoFile(tmp_filename)
  if err != nil {
    return "", errors.New(fmt.Sprintf("Error writing new indexed datastore file %s: %s", tmp_filename, err))
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
    return "", errors.New(fmt.Sprintf("Error renaming %s to %s: %s", tmp_filename, filename, err))
  }

  // Add the new store file to the list of files we know about
  for i, storefile := range this.store_files {
    if storefile.Filename == filename {
      // Remove the file from the list of known files
      storefile.Close()
      this.store_files[i] = nil
      this.store_files = append(this.store_files[:i], this.store_files[i+1:]...)
      break
    }
  }
  newfile := NewIndexedStoreFile(filename)
  this.store_files = append(this.store_files, newfile)
  go func() {
    // Open the file to read the header, then immediately close it to free up filehandles.
    newfile.Open()
    newfile.Close()
  }()
  return filename, nil
}

// indexRecordLog reads in the recordlog file, runlength-encode the ValueStreams, then write out a new datastore file
// with a header identifying where each record lives on disk.
func (this *StoreManager) indexRecordLog(input_filename string, waitgroup *sync.WaitGroup) error {
  this.rotating_mutex.Lock()
  defer this.rotating_mutex.Unlock()
  defer waitgroup.Done()
  log.Printf("Indexing %s", input_filename)
  streams := make(map[string]*oproto.ValueStream, 0)
  var max_timestamp, min_timestamp uint64
  var input_value_count uint64
  this.readRecordLog(input_filename,
    func(new_stream *oproto.ValueStream) {
      new_variable := variable.NewFromProto(new_stream.Variable)
      stream, ok := streams[new_variable.String()]
      if !ok {
        stream = new(oproto.ValueStream)
        stream.Variable = new_variable.AsProto()
        streams[new_variable.String()] = stream
      }

      writer := openinstrument.ValueStreamWriter(stream)
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
        writer <- value
      }
      close(writer)
    })
  log.Printf("Read in recordlog file containing %d streams from %d to %d, %d values",
    len(streams), min_timestamp, max_timestamp, input_value_count)

  _, err := this.writeIndexedFile(streams, min_timestamp, max_timestamp)
  if err != nil {
    return errors.New(fmt.Sprintf("Error writing indexed file: %s", err))
  }
  log.Printf("Removing %s", input_filename)
  if err := os.Remove(input_filename); err != nil {
    return errors.New(fmt.Sprintf("Error removing old recordlog file %s: %s", input_filename, err))
  }
  return nil
}

func (this *StoreManager) writeRecordLog() {
  // Goroutine that runs constantly, appending entries to the recordlog
  tick := time.Tick(1 * time.Minute)
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
        log.Println(err)
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
      if stat.Size() >= *recordlog_max_size*1024*1024 {
        log.Printf("Recordlog is %d MB (>%d MB), rotating", stat.Size()/1024/1024, *recordlog_max_size)
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
      names, err := openinstrument.Readdirnames(*datastore_path)
      if err != nil {
        log.Println(err)
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
  log.Println("Closing record log")
  if file != nil {
    file.Close()
  }
}

func (this *StoreManager) readRecordLog(filename string, callback func(*oproto.ValueStream)) time.Duration {
  start_time := time.Now()
  file, err := openinstrument.ReadProtoFile(filename)
  if err != nil {
    log.Printf("Error opening proto file: %s", err)
    return 0
  }

  // Attempt to read the first proto as a StoreFileHeader
  header := new(oproto.StoreFileHeader)
  n, err := file.Read(header)
  if n != 1 || err != nil {
    // It failed, there is probably no header to this file, try again
    file.Seek(0)
  }

  var stream_count int
  var value_count int
  // Create a ValueStreamReader to continue reading the file in parallel.
  // The channel length supplied was picked roughly at random.
  reader := file.ValueStreamReader(50)
  for stream := range reader {
    callback(stream)
    stream_count++
    value_count += len(stream.Value)
  }
  //log.Printf("Finished reading %d record log streams containing %d values in %v, StoreManager contains %d streams ",
  //stream_count, value_count, duration, len(this.streams))
  file.Close()
  return time.Since(start_time)
}

func (this *StoreManager) GetValueStreams(v *variable.Variable, min_timestamp, max_timestamp *uint64) chan *oproto.ValueStream {
  c := make(chan *oproto.ValueStream, 1000)
  go func() {
    waitgroup := new(sync.WaitGroup)
    waitgroup.Add(1)
    go func() {
      // Return all the streams currently in RAM that match the supplied variable
      // This should be very fast
      this.streams_mutex.RLock()
      defer this.streams_mutex.RUnlock()
      if this.streams == nil {
        this.streams = make(map[string]*oproto.ValueStream, 0)
      }
      for key, stream := range this.streams {
        k := variable.NewFromString(key)
        if k.Match(v) {
          c <- stream
        }
      }
      waitgroup.Done()
    }()
    // Find indexed store files that have data matching the requested date range
    // This may take a *LOT* longer to run
    for _, file := range this.store_files {
      waitgroup.Add(1)
      go func(file *IndexedStoreFile) {
        defer waitgroup.Done()
        if min_timestamp != nil && *min_timestamp > file.MaxTimestamp {
          return
        }
        if max_timestamp != nil && *max_timestamp < file.MinTimestamp {
          return
        }
        for _, stream := range file.GetStreams(v) {
          k := variable.NewFromProto(stream.Variable)
          if k.Match(v) {
            c <- stream
          }
        }
      }(file)
    }
    // Wait for everything to complete.
    // There is no early exit if the receiver doesn't need any more data
    waitgroup.Wait()
    close(c)
  }()
  return c
}

func (this *StoreManager) AddValueStreams() chan *oproto.ValueStream {
  c := make(chan *oproto.ValueStream, 10)
  go func() {
    for new_stream := range c {
      this.addValueStreamNoRecord(new_stream)
      this.record_log_chan <- new_stream
    }
  }()
  return c
}

func (this *StoreManager) addValueStreamNoRecord(new_stream *oproto.ValueStream) {
  this.streams_mutex.RLock()
  if this.streams == nil {
    this.streams = make(map[string]*oproto.ValueStream, 0)
  }
  new_variable := variable.NewFromProto(new_stream.Variable)
  stream, ok := this.streams[new_variable.String()]
  if ok {
    stream.Value = append(stream.Value, new_stream.Value...)
  } else {
    this.streams[new_variable.String()] = new_stream
  }
  this.streams_mutex.RUnlock()
}

func (this *StoreManager) numOpenFiles() (c int) {
  for _, store_file := range this.store_files {
    if store_file.IsOpen() {
      c++
    }
  }
  return
}

func (this *StoreManager) SetAddress(address string) {
  this.address = address
}

func sortedKeys(m map[string]*oproto.ValueStream) []string {
  keys := make([]string, len(m))
  i := 0
  for k := range m {
    keys[i] = k
    i++
  }
  sort.Strings(keys)
  return keys
}
