/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include <google/protobuf/message.h>
#include <string>
#include "lib/common.h"
#include "lib/exported_vars.h"
#include "lib/openinstrument.pb.h"
#include "lib/socket.h"

#ifndef OPENINSTRUMENT_LIB_STORE_CLIENT_H_
#define OPENINSTRUMENT_LIB_STORE_CLIENT_H_

namespace openinstrument {

class StoreClient : private noncopyable {
 public:
  explicit StoreClient(const string &address)
    : address_(address),
      request_timer_("/openinstrument/client/store/all-requests") {}

  explicit StoreClient(const Socket::Address &address)
    : address_(address),
      request_timer_("/openinstrument/client/store/all-requests") {}

  template<typename ResponseType>
  void SendRequest(const string &path, const google::protobuf::Message &request, ResponseType *response);

  proto::AddResponse *Add(const proto::AddRequest &req);
  proto::ListResponse *List(const proto::ListRequest &req);
  proto::GetResponse *Get(const proto::GetRequest &req);
  proto::StoreConfig *PushStoreConfig(const proto::StoreConfig &req);

 private:
  Socket::Address address_;
  ExportedTimer request_timer_;
};

}  // namespace openinstrument

#endif  // OPENINSTRUMENT_LIB_STORE_CLIENT_H_
