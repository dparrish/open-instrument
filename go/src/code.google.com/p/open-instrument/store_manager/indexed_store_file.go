package store_manager

import (
  "code.google.com/p/open-instrument"
  "code.google.com/p/open-instrument/variable"
  "errors"
  "fmt"
  "sort"
  "sync"
  "time"
  openinstrument_proto "code.google.com/p/open-instrument/proto"
)

type IndexedStoreFile struct {
  Filename string
  file *openinstrument.ProtoFileReader
  MinTimestamp uint64
  MaxTimestamp uint64
  header openinstrument_proto.StoreFileHeader
  offsets map[string] uint64
  last_use time.Time
  header_read bool
  close_mutex sync.Mutex
}

type By func(p1, p2 *IndexedStoreFile) bool

func (by By) Sort(files []*IndexedStoreFile) {
  sfs := &indexedStoreFileSorter{
    files: files,
    by: by,
  }
  sort.Sort(sfs)
}

type indexedStoreFileSorter struct {
  files []*IndexedStoreFile
  by By
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
    this.offsets = make(map[string] uint64, len(this.header.Index))
    this.last_use = time.Unix(0, 0)
    for _, index := range this.header.Index {
      varname := variable.NewFromProto(index.Variable).String()
      this.offsets[varname] = index.GetOffset()
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
  this.close_mutex.Lock()
  defer this.close_mutex.Unlock()
  err := this.file.Close()
  this.file = nil
  return err
}

func (this *IndexedStoreFile) GetStreams(v *variable.Variable) []*openinstrument_proto.ValueStream {
  this.close_mutex.Lock()
  defer this.close_mutex.Unlock()
  if this.file == nil {
    this.Open()
  }
  ret := make([]*openinstrument_proto.ValueStream, 0)
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
