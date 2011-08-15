/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_CORD_H_
#define _OPENINSTRUMENT_LIB_CORD_H_

#include <deque>
#include <string>
#include "lib/common.h"
#include "lib/string.h"

namespace openinstrument {

class CordBuffer;
using std::deque;

class Cord : public noncopyable {
 public:
  typedef deque<CordBuffer>::iterator iterator;
  typedef deque<CordBuffer>::const_iterator const_iterator;

  Cord()
    : default_buffer_size_(1024),
      minimum_append_size_(16),
      size_(0) {
  }

  Cord(const Cord &src)
    : default_buffer_size_(src.default_buffer_size_),
      minimum_append_size_(src.minimum_append_size_),
      size_(0) {
    CopyFrom(src);
  }

  ~Cord() {
    clear();
  }

  inline void operator=(const string &str) {
    clear();
    CopyFrom(str);
  }

  inline void set_default_buffer_size(uint32_t newval) {
    default_buffer_size_ = newval;
  }

  inline void set_minimum_append_size(uint32_t newval) {
    minimum_append_size_ = newval;
  }

  bool operator==(const Cord &cord) const;

  inline bool empty() const {
    return size_ == 0;
  }

  // Return the total amount of space in the Cord which has been allocated but not used (and may be unavailable for use)
  uint64_t WastedSpace() const;

  // Append a copy of the supplied data to the Cord
  void CopyFrom(const char *buf, uint32_t size);
  void CopyFrom(const Cord &src);
  inline void CopyFrom(const string &str) {
    CopyFrom(str.data(), str.size());
  }

  // Append a block to the Cord. Copies of the data are not made to all care must be made not to delete the underlying
  // objects before the Cord is deleted.
  void Append(const char *buf, size_t size);
  void Append(const StringPiece &src) {
    Append(src.data(), src.size());
  }
  inline void Append(const string &str) {
    Append(str.data(), str.size());
  }

  void GetAppendBuf(uint32_t reqsize, char **buffer, uint32_t *realsize);
  inline uint64_t size() const {
    return size_;
  }

  void AppendTo(string *str) const;
  string ToString() const;

  char at(uint64_t start) const;
  inline char operator[](uint64_t start) const {
    return at(start);
  }

  void Substr(uint64_t start, uint64_t len, string *out) const;
  string Substr(uint64_t start, uint64_t len) const;

  // Return a string containing the first full line (terminated by [\r\n]) in the Cord.
  // Throws out_of_range if there is no complete line available.
  void ConsumeLine(string *output);

  // Return a string containing the first <butes> bytes of the Cord.
  // Throws out_of_range if there are not enough bytes available.
  void Consume(uint64_t bytes, string *output);

  // STL container methods
  void clear();
  void pop_back();
  void pop_front();

  inline CordBuffer &front() {
    return buffers_.front();
  }

  inline CordBuffer &back() {
    return buffers_.back();
  }

  inline const CordBuffer &front() const {
    return buffers_.front();
  }

  inline const CordBuffer &back() const {
    return buffers_.back();
  }

  inline iterator begin() {
    return buffers_.begin();
  }

  inline iterator end() {
    return buffers_.end();
  }

 private:
  deque<CordBuffer> buffers_;
  uint32_t default_buffer_size_;
  uint32_t minimum_append_size_;
  uint64_t size_;
};


class CordBuffer {
 public:
  CordBuffer()
    : buffer_(NULL),
      mutable_buffer_(NULL),
      size_(0),
      capacity_(0),
      used_(0),
      dealloc_(false) {}

  explicit CordBuffer(uint32_t size)
    : buffer_(NULL),
      mutable_buffer_(NULL),
      size_(0),
      capacity_(0),
      used_(0),
      dealloc_(true) {
    Alloc(size);
  }

  explicit CordBuffer(const char *ptr, uint32_t size)
    : buffer_(ptr),
      mutable_buffer_(NULL),
      size_(size),
      capacity_(size),
      used_(0),
      dealloc_(false) {}

  explicit CordBuffer(const CordBuffer &src)
    : buffer_(src.buffer_),
      mutable_buffer_(NULL),
      size_(src.size_),
      capacity_(src.capacity_),
      used_(src.used_),
      dealloc_(src.dealloc_) {
    if (src.mutable_buffer_) {
      Alloc(src.capacity_);
      if (src.size_) {
        memcpy(mutable_buffer_, src.mutable_buffer_, src.size_);
      }
    }
  }

  virtual ~CordBuffer() {
    if (dealloc_ && mutable_buffer_) {
      delete[] mutable_buffer_;
    }
  }

  virtual bool dealloc() const {
    return dealloc_;
  }

  virtual void Alloc(uint32_t size) {
    capacity_ = size - 1;
    capacity_ |= (capacity_ >> 1);
    capacity_ |= (capacity_ >> 2);
    capacity_ |= (capacity_ >> 4);
    capacity_ |= (capacity_ >> 8);
    capacity_ |= (capacity_ >> 16);
    capacity_++;
    mutable_buffer_ = new char[capacity_];
    dealloc_ = true;
  }

  virtual uint32_t size() const {
    return size_ - used_;
  }

  virtual uint32_t Available() const {
    return capacity_ - size_;
  }

  virtual const char *buffer() const {
    return dealloc_ ? mutable_buffer_ + used_ : buffer_ + used_;
  }

  virtual char *mutable_buffer() const {
    return dealloc_ ? mutable_buffer_ + used_ : NULL;
  }

  uint32_t capacity() const {
    return capacity_;
  }

  void Consume(uint32_t bytes) {
    used_ += bytes;
    if (used_ > size_)
      used_ = size_;
  }

 protected:
  const char *buffer_;
  char *mutable_buffer_;
  uint32_t size_;
  uint32_t capacity_;
  uint32_t used_;
  bool dealloc_;

  virtual bool Use(uint32_t size) {
    if (size_ + size > capacity_)
      return false;
    size_ += size;
    return true;
  }

  friend class Cord;
};

}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_CORD_H_
