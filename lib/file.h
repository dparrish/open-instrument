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
#include <fcntl.h>
#include <glob.h>
#include <poll.h>
#include <stdio.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>
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

class File {
 public:
  File(const string &filename, const char *mode = "r");
  virtual ~File();
  bool Open(const char *mode);
  void Close();

  template<typename T> int32_t Read(T *ptr, uint32_t size) {
    if (!fd_)
      return 0;
    return read(fd_, reinterpret_cast<char *>(ptr), size);
  }

  int32_t Write(const char *ptr, uint32_t size);
  int32_t Write(const string &str);

  template<typename T> int32_t Write(const T &val, uint32_t size) {
    if (!fd_)
      return 0;
    return write(fd_, reinterpret_cast<const char *>(&val), size);
  }

  virtual int64_t SeekAbs(int64_t offset);
  virtual int64_t SeekRel(int64_t offset);
  virtual int64_t Tell() const;

  int fd() const {
    return fd_;
  }

  const string &filename() const {
    return filename_;
  }

  const FileStat &stat() {
    if (!stat_.get())
      stat_.reset(new FileStat(filename_));
    return *stat_;
  }

 protected:
  scoped_ptr<FileStat> stat_;
  string filename_;
  int fd_;
};

class MmapFile : public File {
 public:
  MmapFile(const string &filename);
  virtual ~MmapFile();
  bool Open(const char *mode);
  void Close();

  template<typename T> int32_t Read(T *ptr, uint32_t len) {
    int32_t ret = Read(pos_, len, reinterpret_cast<char *>(ptr));
    if (ret > 0)
      pos_ += ret;
    return ret;
  }

  virtual int32_t Read(uint64_t start, uint32_t len, char *buf);
  virtual StringPiece Read(uint64_t start, uint32_t len);

  virtual int64_t SeekAbs(int64_t offset) {
    pos_ = offset;
    return pos_;
  }

  virtual int64_t SeekRel(int64_t offset) {
    pos_ += offset;
    return pos_;
  }

  virtual int64_t Tell() const {
    return pos_;
  }

 private:
  uint64_t size_;
  uint64_t pos_;
  char *ptr_;
};

vector<string> Glob(const string &pattern);

class FilesystemWatcher : private noncopyable {
 public:
  typedef boost::function<void(const string &)> EventCallback;
  enum EventType {
    FILE_WRITTEN = IN_CLOSE_WRITE,
    FILE_RENAMED = IN_MOVED_TO,
    FILE_DELETED = IN_DELETE,
  };

  FilesystemWatcher()
    : fd_(inotify_init1(O_NONBLOCK | O_CLOEXEC)),
      background_thread_(bind(&FilesystemWatcher::Watcher, this)) {
  }

  ~FilesystemWatcher() {
    if (fd_)
      close(fd_);
    fd_ = 0;
    background_thread_.interrupt();
    background_thread_.join();
  }


  bool SyncWatch(const string &path, EventType mask) {
    Notification notify;
    AddWatch(path, mask, bind(&Notification::Notify, &notify));
    notify.WaitForNotification();
    RemoveWatch(path);
    return true;
  }

 private:
  struct watch_callback;
 public:
  bool AddWatch(const string &path, EventType mask, EventCallback callback) {
    int wd = inotify_add_watch(fd_, path.c_str(), static_cast<uint32_t>(mask));
    if (wd < 0) {
      LOG(ERROR) << "Error adding inotify watch: " << strerror(errno);
      return false;
    }
    struct watch_callback cb;
    cb.path = path;
    cb.callback = callback;
    watches_[wd] = cb;
    return true;
  }

  void RemoveWatch(const string &path) {
    for (auto watch : watches_) {
      if (watch.second.path == path) {
        inotify_rm_watch(fd_, watch.first);
        watches_.erase(watch.first);
        return;
      }
    }
  }

 private:
  void Watcher() {
    char buf[4096];
    while (fd_) {
      struct pollfd fds[1];
      fds[0].fd = fd_;
      fds[0].events = POLLIN;
      poll(fds, 1, 1000);
      if (!(fds[0].revents & POLLIN))
        continue;
      ssize_t bytes = read(fd_, buf, sizeof(buf));
      if (bytes <= 0) {
        if (errno == EAGAIN)
          continue;
        LOG(WARNING) << "FilesystemWatcher::Watcher read returned " << strerror(errno);
        break;
      }
      struct inotify_event *event = reinterpret_cast<struct inotify_event *>(buf);
      while (event < reinterpret_cast<struct inotify_event *>(buf + bytes)) {
        VLOG(2) << "FilesystemWatcher event.wd: " << event->wd;
        VLOG(2) << "FilesystemWatcher event.mask: " << event->mask;
        VLOG(2) << "FilesystemWatcher event.cookie: " << event->cookie;
        if (event->len)
          VLOG(2) << "FilesystemWatcher event.name: " << event->name;
        for (auto watch : watches_) {
          if (watch.first == event->wd) {
            string filename(event->name, event->len);
            watch.second.callback(filename);
          }
        }
        event += event->len + sizeof(struct inotify_event);
      }
    }
  }

  struct watch_callback {
    string path;
    EventCallback callback;
  };
  int fd_;
  unordered_map<int, watch_callback> watches_;
  thread background_thread_;
};

}  // namespace openinstrument

#endif  // OPENINSTRUMENT_LIB_FILE_H_
