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

type IndexedStoreFile struct {
  Filename     string
  file         *openinstrument.ProtoFileReader
  MinTimestamp uint64
  MaxTimestamp uint64
  header       openinstrument_proto.StoreFileHeader
  offsets      map[string]uint64
  last_use     time.Time
  header_read  bool
  bloomfilter  *bloom.BloomFilter
  in_use       sync.RWMutex
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

func (this *IndexedStoreFile) Open() error {
  if this.file != nil {
    return nil
  }
  this.in_use.RLock()
  defer this.in_use.RUnlock()
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
    this.last_use = time.Unix(0, 0)

    // Build a Bloom filter containing just the variable names.
    // This will be used to speed lookups.
    this.bloomfilter = bloom.NewWithEstimates(uint(len(this.header.Index)), 0.1)
    for _, index := range this.header.Index {
      varname := variable.NewFromProto(index.Variable)
      this.offsets[varname.String()] = index.GetOffset()
      this.bloomfilter.Add([]byte(varname.Variable))
    }
    /*
       log.Printf("Opened indexed store file %s with data from %s to %s",
                  this.Filename, time.Unix(int64(this.MinTimestamp / 1000), 0),
                  time.Unix(int64(this.MaxTimestamp / 1000), 0))
    */
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
  return err
}

func (this *IndexedStoreFile) GetStreams(v *variable.Variable) []*openinstrument_proto.ValueStream {
  this.in_use.RLock()
  defer this.in_use.RUnlock()
  if this.file == nil {
    this.Open()
  }
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
      stream := new(openinstrument_proto.ValueStream)
      this.file.ReadAt(int64(offset), stream)
      ret = append(ret, stream)
      this.last_use = time.Now()
    }
  }
  return ret
}