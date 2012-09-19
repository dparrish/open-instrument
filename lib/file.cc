/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#define _FILE_OFFSET_BITS 64

#include <string>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "lib/common.h"
#include "lib/file.h"
#include "lib/string.h"

namespace openinstrument {

File::File(const string &filename, const char *mode)
  : stat_(NULL),
    filename_(filename),
    fd_(0) {
  Open(mode);
}

File::~File() {
  Close();
}

bool File::Open(const char *mode) {
  int flags = O_LARGEFILE;
  mode_t modes = 0666;
  if (strcmp(mode, "r") == 0) {
    flags |= O_RDONLY;
  } else if (strcmp(mode, "r+") == 0) {
    flags |= O_RDWR;
  } else if (strcmp(mode, "w") == 0) {
    flags |= O_WRONLY | O_CREAT | O_TRUNC;
  } else if (strcmp(mode, "w+") == 0) {
    flags |= O_RDWR | O_CREAT | O_TRUNC;
  } else if (strcmp(mode, "a") == 0) {
    flags |= O_WRONLY | O_CREAT;
  } else if (strcmp(mode, "a+") == 0) {
    flags |= O_RDWR | O_CREAT;
  }

  if ((fd_ = open(filename_.c_str(), flags, modes)) <= 0)
    throw runtime_error(StringPrintf("Can't open file %s: %s", filename_.c_str(), strerror(errno)));
  if (mode[0] == 'a')
    lseek64(fd_, 0, SEEK_END);
  return true;
}

void File::Close() {
  if (fd_)
    close(fd_);
  fd_ = 0;
}

int32_t File::Write(const char *ptr, int32_t size) {
  if (!fd_)
    throw runtime_error("Attempt to write to closed filehandle");
  return write(fd_, ptr, size);
}

int32_t File::Write(const string &str) {
  if (!fd_)
    throw runtime_error("Attempt to write to closed filehandle");
  return Write(str.data(), str.size());
}

int64_t File::SeekAbs(int64_t offset) {
  if (!fd_)
    throw runtime_error("Attempt to seek on closed filehandle");
  return lseek64(fd_, offset, SEEK_SET);
}

int64_t File::SeekRel(int64_t offset) {
  if (!fd_)
    throw runtime_error("Attempt to seek on closed filehandle");
  return lseek64(fd_, offset, SEEK_CUR);
}

int64_t File::Tell() const {
  if (!fd_)
    throw runtime_error("Attempt to tell on closed filehandle");
  return lseek64(fd_, 0, SEEK_CUR);
}

MmapFile::MmapFile(const string &filename)
  : fh_(NULL),
    size_(0),
    ptr_(NULL) {
  fh_.reset(new File(filename, "r"));
  if (!fh_.get()) {
    LOG(ERROR) << "open() failed: " << strerror(errno);
    return;
  }
  size_ = fh_->stat().size();
  if (!size_) {
    LOG(ERROR) << "empty file: " << strerror(errno);
    fh_->Close();
    return;
  }
  ptr_ = static_cast<char *>(mmap(NULL, size_, PROT_READ, MAP_SHARED, fh_->fd(), 0));
  if (ptr_ <= 0) {
    LOG(ERROR) << "mmap() failed: " << strerror(errno);
    ptr_ = NULL;
    Close();
  }
}

MmapFile::~MmapFile() {
  Close();
}

void MmapFile::Close() {
  if (ptr_ && size_)
    munmap(ptr_, size_);
  if (fh_.get()) {
    fh_->Close();
    fh_.reset(NULL);
  }
  ptr_ = NULL;
  size_ = 0;
}

size_t MmapFile::Read(size_t start, size_t len, char *buf) {
  StringPiece piece = Read(start, len);
  if (piece.size())
    memcpy(buf, piece.data(), piece.size());
  return piece.size();
}

StringPiece MmapFile::Read(size_t start, size_t len) {
  if (!ptr_)
    return StringPiece(NULL, 0);
  if (start > size_)
    return StringPiece(NULL, 0);
  if (len > size_)
    len = size_;
  if (len == 0)
    return StringPiece(NULL, 0);
  if (start + len > size_)
    len = size_ - start;
  if (len <= 0)
    return StringPiece(NULL, 0);
  return StringPiece(ptr_ + start, len);
}


vector<string> Glob(const string &pattern) {
  vector<string> files;
  glob_t pglob;
  if (::glob(pattern.c_str(), 0, NULL, &pglob) == 0) {
    files.reserve(pglob.gl_pathc + 1);
    for (size_t i = 0; i < pglob.gl_pathc; i++)
      files.push_back(pglob.gl_pathv[i]);
  }
  ::globfree(&pglob);
  return files;
}

}  // namespace openinstrument
