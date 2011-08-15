/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef OPENINSTRUMENT_LIB_RPC_CLIENT_H_
#define OPENINSTRUMENT_LIB_RPC_CLIENT_H_

#include "lib/common.h"
#include "lib/exported_vars.h"
#include "lib/http_client.h"
#include "lib/request_handler.h"
#include "lib/socket.h"

namespace openinstrument {
namespace rpc {

class RpcClient {
 public:
  RpcClient() {}
};

}  // namespace rpc
}  // namespace openinstrument

#endif  // _OPENINSTRUMENT_LIB_RPC_CLIENT_H_
