/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef OPENINSTRUMENT_LIB_RPC_SERVER_H_
#define OPENINSTRUMENT_LIB_RPC_SERVER_H_

#include <string>
#include "lib/common.h"
#include "lib/exported_vars.h"
#include "lib/http_server.h"
#include "lib/protobuf.h"
#include "lib/request_handler.h"
#include "lib/socket.h"

namespace openinstrument {
namespace rpc {

class RpcServer {
 public:
  explicit RpcServer(http::HttpServer *server)
    : server_(server) {
    server_->request_handler()->AddPath("/__START_RPC__", &RpcServer::HandleRpc, this);
  }

  template<typename RQ, typename RP>
  void AddHandler(const string &request_name, boost::function<void(const RQ &, RP *)> handler) {
  }

 private:
  bool HandleRpc(const http::HttpRequest &request, http::HttpReply *reply) {
    google::protobuf::Message req;
    if (!UnserializeProtobuf(request.body(), &req)) {
      reply->SetStatus(HttpReply::BAD_REQUEST);
      reply->mutable_body()->clear();
      reply->mutable_body()->CopyFrom("Invalid request.\n");
      return true;
    }
    if (req.GetDescriptor()->full_name() != "openinstrument.proto.GetRequest")
      throw runtime_error("Invalid request");

    if (!SerializeProtobuf(response, reply->mutable_body())) {
      reply->SetStatus(HttpReply::INTERNAL_SERVER_ERROR);
      reply->mutable_body()->clear();
      reply->mutable_body()->CopyFrom("Error serializing protobuf\n");
    }
  }

  http::HttpServer *server_;
};

}  // namespace rpc
}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_RPC_SERVER_H_
