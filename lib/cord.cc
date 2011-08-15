/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120 et
 *
 */

#include <algorithm>
#include <deque>
#include <string>
#include "lib/common.h"
#include "lib/cord.h"
#include "lib/string.h"

namespace openinstrument {

using std::deque;

bool Cord::operator==(const Cord &cord) const {
  deque<CordBuffer>::const_iterator left = buffers_.begin();
  deque<CordBuffer>::const_iterator right = cord.buffers_.begin();
  if (cord.size() != size())
    return false;
  while (left != buffers_.end() && right != cord.buffers_.end()) {
    if (left->size() != right->size())
      return false;

    if (strncmp(left->buffer(), right->buffer(), left->size() != 0))
      return false;

    ++left;
    ++right;
  }
  return true;
}

void Cord::clear() {
  buffers_.clear();
  size_ = 0;
}

void Cord::CopyFrom(const char *buf, uint32_t size) {
  for (uint32_t start = 0; start < size; ) {
    uint32_t bleft = (size - start);
    uint32_t realsize;
    char *buffer;
    GetAppendBuf(bleft, &buffer, &realsize);
    memcpy(buffer, buf + start, realsize);
    start += realsize;
  }
}

void Cord::CopyFrom(const Cord &src) {
  BOOST_FOREACH(const CordBuffer &i, src.buffers_) {
    if (i.dealloc()) {
      // Duplicate the block
      for (uint32_t start = 0; start < i.size(); ) {
        uint32_t bleft = (i.size() - start);
        uint32_t realsize;
        char *buffer;
        GetAppendBuf(bleft, &buffer, &realsize);
        memcpy(buffer, i.buffer() + start, realsize);
        start += realsize;
      }
    } else {
      // Point to the same block
      buffers_.push_back(CordBuffer(i.buffer(), i.size()));
      size_ += i.size();
    }
  }
}

void Cord::Append(const char *buf, size_t size) {
  buffers_.push_back(CordBuffer(buf, size));
  size_ += size;
}

void Cord::GetAppendBuf(uint32_t reqsize, char **buffer, uint32_t *realsize) {
  *realsize = reqsize;
  if (!buffers_.size()) {
    buffers_.push_back(CordBuffer(std::max(default_buffer_size_, reqsize)));
  } else if (buffers_.back().Available() >= reqsize) {
    // There's enough space, return a pointer to the existing buffer
  } else if (buffers_.back().Available() >= minimum_append_size_) {
    *realsize = buffers_.back().Available();
  } else {
    buffers_.push_back(CordBuffer(std::max(default_buffer_size_, reqsize)));
  }
  *buffer = buffers_.back().mutable_buffer() + buffers_.back().size();
  buffers_.back().Use(*realsize);
  size_ += *realsize;
}

void Cord::AppendTo(string *str) const {
  BOOST_FOREACH(const CordBuffer &i, buffers_) {
    str->append(i.buffer(), i.size());
  }
}

string Cord::ToString() const {
  string out;
  AppendTo(&out);
  return out;
}

char Cord::at(uint64_t start) const {
  if (start >= size_)
    throw out_of_range("Out of range");
  BOOST_FOREACH(const CordBuffer &i, buffers_) {
    if (start > i.size()) {
      // Start position is not inside this block
      start -= i.size();
      continue;
    }
    // Found a block containing the character
    return *(i.buffer() + start);
  }
  throw runtime_error("Cord::at() failed to find a character");
}

void Cord::Substr(uint64_t start, uint64_t len, string *out) const {
  if (start >= size_)
    throw out_of_range("Out of range");
  uint64_t reallen = std::min(size_ - start, len);
  out->reserve(out->size() + reallen);
  BOOST_FOREACH(const CordBuffer &i, buffers_) {
    if (start > i.size()) {
      // Start position is not inside this block
      start -= i.size();
      continue;
    }
    // Found a block containing some of the data we want
    uint32_t num_b = std::min(reallen, i.size() - start);
    out->append(i.buffer() + start, num_b);
    reallen -= num_b;
    if (reallen <= 0)
      break;
    start = 0;
  }
}

string Cord::Substr(uint64_t start, uint64_t len) const {
  string out;
  Substr(start, len, &out);
  return out;
}

void Cord::pop_back() {
  size_ -= buffers_.back().size();
  buffers_.pop_back();
}

void Cord::pop_front() {
  size_ -= buffers_.front().size();
  buffers_.pop_front();
}

void Cord::ConsumeLine(string *output) {
  if (!size_)
    throw out_of_range("Empty Cord");
  uint64_t bytes_used = 0;
  bool found_newline = 0;
  BOOST_FOREACH(CordBuffer &buf, buffers_) {
    if (!buf.size()) {
      buffers_.pop_front();
      continue;
    }
    for (const char *p = buf.buffer(); p < buf.buffer() + buf.size(); p++) {
      if (*p == '\n') {
        // End of line
        if (output)
          output->append(buf.buffer(), p - buf.buffer() + 1);
        bytes_used += p - buf.buffer() + 1;
        found_newline = true;
        break;
      }
    }
    if (found_newline)
      break;
    if (output)
      output->append(buf.buffer(), buf.size());
    bytes_used += buf.size();
  }
  if (!found_newline)
    throw out_of_range("No entire line found");

  try {
    Consume(bytes_used, NULL);
  } catch (exception) {
    // Ignore
  }

  if (output) {
    int trim_chars = 0;
    for (string::reverse_iterator i = output->rbegin(); i != output->rend(); ++i) {
      if (*i == '\n' || *i == '\r')
        trim_chars++;
      else
        break;
    }
    if (trim_chars)
      output->resize(output->size() - trim_chars);
  }
}

void Cord::Consume(uint64_t bytes, string *output) {
  if (bytes > size_)
    throw out_of_range("Empty Cord");
  if (bytes == size_) {
    // Short-cut when the entire Cord is requested
    if (output)
      AppendTo(output);
    clear();
    return;
  }
  uint64_t bytes_left = bytes;
  if (output)
    output->reserve(output->size() + bytes);
  BOOST_FOREACH(CordBuffer &buf, buffers_) {
    if (bytes_left == 0 || bytes_left > bytes)
      break;
    if (buf.size() >= bytes_left) {
      // There's enough left in this block
      if (output)
        output->append(buf.buffer(), bytes_left);
      buf.Consume(bytes_left);
      size_ -= bytes_left;
      if (buf.size() == 0)
        buffers_.pop_front();
      bytes_left = 0;
      break;
    } else {
      // There's not enough bytes in this block, append what we have and move on
      if (output)
        output->append(buf.buffer(), buf.size());
      bytes_left -= buf.size();
      buffers_.pop_front();
    }
  }
  if (bytes_left != 0)
    throw runtime_error("Something went wrong, wrong number of bytes were returned from Cord::Consume");
}

}  // namespace openinstrument
