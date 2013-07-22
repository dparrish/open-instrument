package store_manager

import (
  "code.google.com/p/open-instrument"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
  "code.google.com/p/open-instrument/variable"
  "errors"
  "fmt"
  "github.com/willf/bloom"
  "sort"
  "strings"
  "sync"
  "time"
)

// Hard cap on the number of open indexed files
var max_files_semaphore = make(openinstrument.Semaphore, 500)

type IndexedStoreFile struct {
  Filename     string
  FileSize     int64
  file         *openinstrument.ProtoFileReader
  MinTimestamp uint64
  MaxTimestamp uint64
  header       openinstrument_proto.StoreFileHeader
  offsets      map[string]uint64
  last_use     time.Time
  header_read  bool
  bloomfilter  *bloom.BloomFilter
  in_use       sync.RWMutex
  stream_cache map[string]*openinstrument_proto.ValueStream
}

type By func(p1, p2 *IndexedStoreFile) bool

func (by By) Sort(files []*IndexedStoreFile) {
  sfs := &indexedStoreFileSorter{
    files: files,
    by:    by,
  }
  sort.Sort(sfs)
}

type indexedStoreFileSorter struct {
  files []*IndexedStoreFile
  by    By
}

func (this *indexedStoreFileSorter) Len() int {
  return len(this.files)
}

func (this *indexedStoreFileSorter) Swap(i, j int) {
  this.files[i], this.files[j] = this.files[j], this.files[i]
}

func (this *indexedStoreFileSorter) Less(i, j int) bool {
  return this.by(this.files[i], this.files[j])
}

func (this *IndexedStoreFile) String() string {
  return this.Filename
}

func NewIndexedStoreFile(filename string) *IndexedStoreFile {
  return &IndexedStoreFile{
    Filename:     filename,
    stream_cache: make(map[string]*openinstrument_proto.ValueStream),
  }
}

func (this *IndexedStoreFile) IsOpen() bool {
  return this.file != nil
}

func (this *IndexedStoreFile) LastUse() time.Time {
  return this.last_use
}

func (this *IndexedStoreFile) Open() error {
  this.last_use = time.Now()
  if this.file != nil {
    return nil
  }
  max_files_semaphore.Lock()
  this.in_use.Lock()
  defer this.in_use.Unlock()
  var err error
  this.file, err = openinstrument.ReadProtoFile(this.Filename)
  if err != nil {
    return errors.New(fmt.Sprintf("Error opening indexed store file %s: %s", this.Filename, err))
  }

  if !this.header_read {
    n, err := this.file.Read(&this.header)
    if n != 1 || err != nil {
      this.file.Close()
      this.file = nil
      return errors.New(fmt.Sprintf("Can't read header from indexed store file %s: %s", this.Filename, err))
    }
    this.MinTimestamp = this.header.GetStartTimestamp()
    this.MaxTimestamp = this.header.GetEndTimestamp()
    this.offsets = make(map[string]uint64, len(this.header.Index))
    stat, _ := this.file.Stat()
    this.FileSize = stat.Size()

    // Build a Bloom filter containing just the variable names.
    // This will be used to speed lookups.
    this.bloomfilter = bloom.NewWithEstimates(uint(len(this.header.Index)), 0.1)
    for _, index := range this.header.Index {
      varname := variable.NewFromProto(index.Variable)
      this.offsets[varname.String()] = index.GetOffset()
      this.bloomfilter.Add([]byte(varname.Variable))
    }
    this.header_read = true
  }
  return nil
}

func (this *IndexedStoreFile) Close() error {
  if this.file == nil {
    return nil
  }
  this.in_use.Lock()
  defer this.in_use.Unlock()
  err := this.file.Close()
  this.file = nil
  this.stream_cache = make(map[string]*openinstrument_proto.ValueStream)
  max_files_semaphore.Unlock()
  return err
}

func (this *IndexedStoreFile) GetStreams(v *variable.Variable) []*openinstrument_proto.ValueStream {
  this.Open()
  this.in_use.Lock()
  defer this.in_use.Unlock()
  ret := make([]*openinstrument_proto.ValueStream, 0)
  if !strings.HasSuffix(v.Variable, "*") {
    // This is a direct string lookup, check the Bloom filter first
    if !this.bloomfilter.Test([]byte(v.Variable)) {
      return ret
    }
  }
  for key, offset := range this.offsets {
    varmatch := variable.NewFromString(key)
    if varmatch.Match(v) {
      stream, ok := this.stream_cache[key]
      if ok {
        ret = append(ret, stream)
      } else {
        stream = new(openinstrument_proto.ValueStream)
        this.file.ReadAt(int64(offset), stream)
        ret = append(ret, stream)
        this.last_use = time.Now()
        this.stream_cache[key] = stream
      }
    }
  }
  return ret
}
