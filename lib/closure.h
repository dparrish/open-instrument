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

namespace openinstrument {

class Executor {
 public:
  virtual void Run(const boost::function0<void> &f) {
    f();
  }
  virtual ~Executor() {}
};

typedef boost::function<void()> Callback;
class CallbackRunner {
 public:
  CallbackRunner(Callback cb) : cb_(cb) {}
  ~CallbackRunner() {
    if (cb_)
      cb_();
  }
 private:
  Callback cb_;
};

/*
class Closure {
 public:
  typedef boost::function<void()> FuncType;
  Closure(FuncType func) : func_(func) {}
  virtual ~Closure() {}
  virtual void Run() = 0;
 protected:
  FuncType func_;
};

Closure *NewCallback(Closure::FuncType fun);
Closure *NewPermanentCallback(Closure::FuncType fun);

class ClosureRunner {
 public:
  ClosureRunner(Closure *done) : done_(done) {}
  ~ClosureRunner() {
    if (done_)
      done_->Run();
  }
 private:
  Closure *done_;
};
*/

}  // namespace openinstrument

#endif  //_OPENINSTRUMENT_LIB_COMMON_H_
