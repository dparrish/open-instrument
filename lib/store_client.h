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

class StoreConfig;

class StoreClient : private noncopyable {
 public:
  // Preferred method of connecting to the storage server cluster.
  // The caller is responsible for deleting the config object.
  explicit StoreClient(StoreConfig *config);

  // Connect to a single storage server
  explicit StoreClient(const string &address);
  explicit StoreClient(const Socket::Address &address);

  void SendRequestToHost(const Socket::Address &address, const string &path, const google::protobuf::Message &request,
                         google::protobuf::Message *response);

  template<typename ResponseType>
  void SendRequest(const string &path, const google::protobuf::Message &request, ResponseType *response);

  proto::AddResponse *Add(const proto::AddRequest &req);
  proto::ListResponse *List(const proto::ListRequest &req);
  proto::GetResponse *Get(const proto::GetRequest &req);
  proto::StoreConfig *GetStoreConfig();

 private:
  StoreConfig *store_config_;
  Socket::Address address_;
  string hostname_;
  ExportedTimer request_timer_;
};

}  // namespace openinstrument

#endif  // OPENINSTRUMENT_LIB_STORE_CLIENT_H_
