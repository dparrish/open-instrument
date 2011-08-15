/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <boost/function.hpp>
#include "lib/closure.h"

namespace openinstrument {

/*
class OnceClosure : public Closure {
 public:
  OnceClosure(FuncType func) : Closure(func) {}
  void Run() {
    func_();
    delete this;
  }
};

class PermanentClosure : public Closure {
 public:
  PermanentClosure(FuncType func) : Closure(func) {}
  void Run() {
    func_();
  }
};

Closure *NewCallback(Closure::FuncType fun) {
  return new OnceClosure(fun);
}

Closure *NewPermanentCallback(Closure::FuncType fun) {
  return new PermanentClosure(fun);
}
*/

}  // namespace openinstrument
