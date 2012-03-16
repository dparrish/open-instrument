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
#include "lib/http_client.h"
#include "lib/openinstrument.pb.h"
#include "lib/protobuf.h"
#include "lib/socket.h"
#include "lib/store_client.h"
#include "lib/string.h"
#include "lib/uri.h"

namespace openinstrument {

using http::HttpClient;
using http::HttpReply;
using http::HttpRequest;
using http::Uri;

template<typename ResponseType>
void StoreClient::SendRequest(const string &path, const google::protobuf::Message &request, ResponseType *response) {
  ScopedExportTimer t(&request_timer_);

  if (address_.address == 0)
    throw runtime_error("Invalid server host");
  if (!address_.port)
    throw runtime_error("Invalid server port");

  Uri uri;
  uri.set_protocol("http");
  uri.set_hostname(address_.AddressToString());
  uri.set_port(address_.port);
  uri.set_path(path);

  HttpClient client;
  scoped_ptr<HttpRequest> req(client.NewRequest(uri.Assemble()));
  SerializeProtobuf(request, req->mutable_body());
  req->set_method("POST");
  req->SetContentLength(req->body().size());
  scoped_ptr<HttpReply> reply(client.SendRequest(*req));

  if (!UnserializeProtobuf(reply->body(), response))
    throw runtime_error("Invalid response from the server");
}

proto::AddResponse *StoreClient::Add(const proto::AddRequest &req) {
  scoped_ptr<proto::AddResponse> response(new proto::AddResponse());
  SendRequest("/add", req, response.get());
  if (!response->success())
    throw runtime_error(StringPrintf("Server Error: %s", response->errormessage().c_str()));
  return response.release();
}

proto::ListResponse *StoreClient::List(const proto::ListRequest &req) {
  scoped_ptr<proto::ListResponse> response(new proto::ListResponse());
  SendRequest("/list", req, response.get());
  if (!response->success())
    throw runtime_error(StringPrintf("Server Error: %s", response->errormessage().c_str()));
  return response.release();
}

proto::GetResponse *StoreClient::Get(const proto::GetRequest &req) {
  scoped_ptr<proto::GetResponse> response(new proto::GetResponse());
  SendRequest("/get", req, response.get());
  if (!response->success())
    throw runtime_error(StringPrintf("Server Error: %s", response->errormessage().c_str()));
  return response.release();
}

proto::StoreConfig *StoreClient::PushStoreConfig(const proto::StoreConfig &req) {
  scoped_ptr<proto::StoreConfig> response(new proto::StoreConfig());
  SendRequest("/push_config", req, response.get());
  return response.release();
}

}  // namespace openinstrument
