package openinstrument

import (
  "code.google.com/p/goprotobuf/proto"
  "encoding/binary"
  "errors"
  "fmt"
  "github.com/joaojeronimo/go-crc16"
  "io"
  "log"
  "os"
)

var PROTO_MAGIC uint16 = 0xDEAD

type ProtoFileReader struct {
  filename string
  file     *os.File
  stat     os.FileInfo
}

func ReadProtoFile(filename string) (*ProtoFileReader, error) {
  reader := new(ProtoFileReader)
  reader.filename = filename
  var err error
  reader.file, err = os.Open(filename)
  if err != nil {
    return nil, err
  }
  reader.stat, err = reader.file.Stat()
  if err != nil {
    reader.file.Close()
    return nil, err
  }
  return reader, nil
}

func (this *ProtoFileReader) Close() error {
  return this.file.Close()
}

func (this *ProtoFileReader) Tell() int64 {
  pos, _ := this.file.Seek(0, os.SEEK_CUR)
  return pos
}

func (this *ProtoFileReader) Stat() (os.FileInfo, error) {
  return this.file.Stat()
}

func (this *ProtoFileReader) ReadAt(pos int64, message proto.Message) (int, error) {
  this.file.Seek(pos, os.SEEK_SET)
  return this.Read(message)
}

func (this *ProtoFileReader) Read(message proto.Message) (int, error) {
  var failcount int
  for {
    if failcount > 30 {
      return 0, io.EOF
    }
    pos := this.Tell()
    type header struct {
      Magic  uint16
      Length uint32
    }
    var h header

    err := binary.Read(this.file, binary.LittleEndian, &h)
    if err != nil {
      if err == io.EOF {
        return 0, io.EOF
      }
      log.Printf("Error reading record header from recordlog: %s", err)
      return 0, err
    }

    // Read Magic header
    if h.Magic != PROTO_MAGIC {
      log.Printf("Protobuf delimeter at %d does not match %#x", pos, PROTO_MAGIC)
      failcount++
      continue
    }
    if int64(h.Length) >= this.stat.Size() {
      log.Printf("Chunk length %d is greater than file size %d", h.Length, this.stat.Size())
      failcount++
      continue
    }

    // Read Proto
    buf := make([]byte, h.Length)
    n, err := this.file.Read(buf)
    if err != nil || uint32(n) != h.Length {
      log.Printf("Could not read %d bytes from file: %s", h.Length, err)
      return 0, io.EOF
    }

    // Read CRC
    var crc uint16
    err = binary.Read(this.file, binary.LittleEndian, &crc)
    if err != nil {
      log.Printf("Error reading CRC from recordlog: %s", err)
      return 0, io.EOF
    }
    checkcrc := crc16.Crc16(string(buf))
    if checkcrc != crc {
      //log.Printf("CRC %x does not match %x", crc, checkcrc)
    }

    // Decode and add proto
    if err = proto.Unmarshal(buf, message); err != nil {
      log.Printf("Error decoding protobuf: %s", err)
      return 0, io.EOF
    }
    break
  }
  return 1, nil
}

func WriteProtoFile(filename string) (*ProtoFileWriter, error) {
  reader := new(ProtoFileWriter)
  reader.filename = filename
  var err error
  reader.file, err = os.OpenFile(filename, os.O_RDWR|os.O_CREATE, 0664)
  if err != nil {
    return nil, err
  }
  reader.stat, err = reader.file.Stat()
  if err != nil {
    reader.file.Close()
    return nil, err
  }
  reader.file.Seek(0, os.SEEK_END)
  return reader, nil
}

type ProtoFileWriter struct {
  filename string
  file     *os.File
  stat     os.FileInfo
}

func (this *ProtoFileWriter) Close() error {
  return this.file.Close()
}

func (this *ProtoFileWriter) Tell() int64 {
  pos, _ := this.file.Seek(0, os.SEEK_CUR)
  return pos
}

func (this *ProtoFileWriter) Stat() (os.FileInfo, error) {
  return this.file.Stat()
}

func (this *ProtoFileWriter) WriteAt(pos int64, message proto.Message) (int, error) {
  this.file.Seek(pos, os.SEEK_SET)
  return this.Write(message)
}

func (this *ProtoFileWriter) Write(message proto.Message) (int, error) {
  data, err := proto.Marshal(message)
  if err != nil {
    return 0, errors.New(fmt.Sprintf("Marshaling error: %s", err))
  }
  var buf = []interface{}{
    uint16(PROTO_MAGIC),
    uint32(len(data)),
    data,
    uint16(0),
  }
  for _, v := range buf {
    err = binary.Write(this.file, binary.LittleEndian, v)
    if err != nil {
      return 0, errors.New(fmt.Sprintf("Error writing entry to protofile: %s", err))
    }
  }
  return 1, nil
}
