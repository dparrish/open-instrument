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

BackgroundExecutor::~BackgroundExecutor() {
  JoinThreads();
}

void BackgroundExecutor::Add(const Callback &callback) {
  threads_.push_back(new boost::thread(callback));
  TryJoinThreads();
}

void BackgroundExecutor::TryJoinThreads() {
  for (std::vector<boost::thread *>::iterator i = threads_.begin(); i != threads_.end(); ++i) {
    if ((*i)->timed_join(boost::posix_time::seconds(0))) {
      delete *i;
      threads_.erase(i);
      return TryJoinThreads();
    }
  }
}

void BackgroundExecutor::JoinThreads() {
  for (std::vector<boost::thread *>::iterator i = threads_.begin(); i != threads_.end(); ++i) {
    (*i)->join();
    delete *i;
  }
  threads_.clear();
}

}  // namespace openinstrument
