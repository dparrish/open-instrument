/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef OPENINSTRUMENT_LIB_FILE_H_
#define OPENINSTRUMENT_LIB_FILE_H_

#define _FILE_OFFSET_BITS 64

#include <string>
#include <vector>
#include <glob.h>
#include <stdio.h>
#include <sys/stat.h>
#include "lib/common.h"

namespace openinstrument {

class FileStat {
 public:
  explicit FileStat(const string &filename) {
    if (stat(filename.c_str(), &sb_) < 0) {
      error_ = strerror(errno);
      memset(&sb_, 0, sizeof(sb_));
      exists_ = false;
    } else {
      exists_ = true;
    }
  }

  inline string error() const { return error_; }
  inline bool exists() const { return exists_; }
  inline dev_t dev() const { return sb_.st_dev; }
  inline ino_t ino() const { return sb_.st_ino; }
  inline mode_t mode() const { return sb_.st_mode; }
  inline nlink_t nlink() const { return sb_.st_nlink; }
  inline uid_t uid() const { return sb_.st_uid; }
  inline gid_t gid() const { return sb_.st_gid; }
  inline dev_t rdev() const { return sb_.st_rdev; }
  inline int64_t size() const { return sb_.st_size; }
  inline blksize_t blksize() const { return sb_.st_blksize; }
  inline blkcnt_t blocks() const { return sb_.st_blocks; }
  inline time_t atime() const { return sb_.st_atime; }
  inline time_t mtime() const { return sb_.st_mtime; }
  inline time_t ctime() const { return sb_.st_ctime; }

 protected:
  struct stat sb_;
  bool exists_;
  string error_;
};

class File : private noncopyable {
 public:
  explicit File(const string &filename) : filename_(filename) {
    Stat(filename);
  }

  bool Stat(const string &filename) {
    if (stat(filename.c_str(), &sb_) != 0)
      return false;
    return true;
  }
  inline dev_t dev() const { return sb_.st_dev; }
  inline ino_t ino() const { return sb_.st_ino; }
  inline mode_t mode() const { return sb_.st_mode; }
  inline nlink_t nlink() const { return sb_.st_nlink; }
  inline uid_t uid() const { return sb_.st_uid; }
  inline gid_t gid() const { return sb_.st_gid; }
  inline dev_t rdev() const { return sb_.st_rdev; }
  inline off_t size() const { return sb_.st_size; }
  inline blksize_t blksize() const { return sb_.st_blksize; }
  inline blkcnt_t blocks() const { return sb_.st_blocks; }
  inline time_t atime() const { return sb_.st_atime; }
  inline time_t mtime() const { return sb_.st_mtime; }
  inline time_t ctime() const { return sb_.st_ctime; }

 protected:
  struct stat sb_;
  string filename_;
};

class MmapFile : public File {
 public:
  static MmapFile *Open(const string &filename);
  ~MmapFile();
  void Ref();
  void Deref();
  inline char *data() const { return reinterpret_cast<char *>(ptr_); }

 private:
  explicit MmapFile(const string &filename)
    : File(filename),
      ptr_(NULL),
      refcount_(0) {}
  void *ptr_;
  int16_t refcount_;
  int fd;
};

class MmapFileRef : private noncopyable {
 public:
  explicit MmapFileRef(MmapFile *fh) : fh_(fh) {}
  ~MmapFileRef() {
    if (fh_)
      fh_->Deref();
  }
  inline MmapFile *operator->() const {
    return fh_;
  }
 private:
  MmapFile *fh_;
};

class LocalFile : public File {
 public:
  LocalFile(const string &filename, const char *mode = "r");
  ~LocalFile();
  bool Open(const char *mode);
  void Close();

  template<typename T> int32_t Read(T *ptr, int32_t size) {
    if (!fd_)
      return 0;
    return read(fd_, reinterpret_cast<char *>(ptr), size);
  }

  int32_t Write(const char *ptr, int32_t size);
  int32_t Write(const string &str);

  template<typename T> int32_t Write(const T &val, int32_t size) {
    if (!fd_)
      return 0;
    return write(fd_, reinterpret_cast<const char *>(&val), size);
  }

  int64_t SeekAbs(int64_t offset);
  int64_t SeekRel(int64_t offset);
  int64_t Tell() const;

  int fd() const {
    return fd_;
  }

 private:
  int fd_;
};

vector<string> Glob(const string &pattern);

}  // namespace openinstrument

#endif  // OPENINSTRUMENT_LIB_FILE_H_
