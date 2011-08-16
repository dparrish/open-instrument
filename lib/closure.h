/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef _OPENINSTRUMENT_LIB_CLOSURE_H_
#define _OPENINSTRUMENT_LIB_CLOSURE_H_

#include <boost/function.hpp>
#include "lib/common.h"

namespace openinstrument {

class Executor : public boost::noncopyable {
 public:
  virtual void Run(const boost::function0<void> &f) {
    f();
  }
  virtual ~Executor() {}
};

typedef boost::function<void()> Callback;
class CallbackRunner : public boost::noncopyable {
 public:
  CallbackRunner(Callback cb) : cb_(cb) {}
  ~CallbackRunner() {
    if (cb_)
      cb_();
  }
 private:
  Callback cb_;
};

}  // namespace openinstrument

#endif  //_OPENINSTRUMENT_LIB_COMMON_H_
