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

namespace openinstrument {

MmapFile *MmapFile::Open(const string &filename) {
  scoped_ptr<MmapFile> fh(new MmapFile(filename));
  if (stat(filename.c_str(), &fh->sb_) != 0)
    throw runtime_error(StringPrintf("Could not stat file %s: %s", filename.c_str(), strerror(errno)));

  if (fh->sb_.st_size == 0)
    return NULL;

  fh->fd = open(filename.c_str(), O_RDONLY);
  if (fh->fd <= 0)
    throw runtime_error(StringPrintf("open() failed: %s", strerror(errno)));

  fh->ptr_ = mmap(NULL, fh->size(), PROT_READ, MAP_PRIVATE, fh->fd, 0);
  if (!fh->ptr_)
    throw runtime_error(StringPrintf("mmap() failed: %s", strerror(errno)));
  fh->Ref();
  return fh.release();
}

MmapFile::~MmapFile() {
  if (ptr_)
    munmap(ptr_, size());
  if (fd)
    close(fd);
}

void MmapFile::Ref() {
  ++refcount_;
}

void MmapFile::Deref() {
  if (--refcount_ <= 0) {
    delete this;
  }
}


LocalFile::LocalFile(const string &filename, const char *mode)
  : File(filename),
    fd_(0) {
  Open(mode);
}

LocalFile::~LocalFile() {
  Close();
}

bool LocalFile::Open(const char *mode) {
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

void LocalFile::Close() {
  if (fd_)
    close(fd_);
}

size_t LocalFile::Write(const char *ptr, size_t size) {
  if (!fd_)
    return 0;
  return write(fd_, ptr, size);
}

size_t LocalFile::Write(const string &str) {
  if (!fd_)
    return 0;
  return Write(str.data(), str.size());
}

int64_t LocalFile::SeekAbs(int64_t offset) {
  if (!fd_)
    return 0;
  return lseek64(fd_, offset, SEEK_SET);
}

int64_t LocalFile::SeekRel(int64_t offset) {
  if (!fd_)
    return 0;
  return lseek64(fd_, offset, SEEK_CUR);
}

int64_t LocalFile::Tell() const {
  if (!fd_)
    return 0;
  return lseek64(fd_, 0, SEEK_CUR);
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
