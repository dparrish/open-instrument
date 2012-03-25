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
#include "server/store_config.h"

namespace openinstrument {

using http::HttpClient;
using http::HttpReply;
using http::HttpRequest;
using http::Uri;

// Preferred method of connecting to the storage server cluster.
// The caller is responsible for deleting the config object.
StoreClient::StoreClient(StoreConfig *config)
  : store_config_(config),
    request_timer_("/openinstrument/client/store/all-requests") {}

// Connect to a single storage server
StoreClient::StoreClient(const string &address)
  : store_config_(NULL),
    hostname_(address),
    request_timer_("/openinstrument/client/store/all-requests") {}
StoreClient::StoreClient(const Socket::Address &address)
  : store_config_(NULL),
    address_(address),
    request_timer_("/openinstrument/client/store/all-requests") {}

void StoreClient::SendRequestToHost(const Socket::Address &address, const string &path,
                                    const google::protobuf::Message &request, google::protobuf::Message *response) {
  ScopedExportTimer t(&request_timer_);

  if (!address.valid())
    throw runtime_error("Invalid destination");

  Uri uri;
  uri.set_protocol("http");
  uri.set_hostname(address.AddressToString());
  uri.set_port(address.port());
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

template<typename ResponseType>
void StoreClient::SendRequest(const string &path, const google::protobuf::Message &request, ResponseType *response) {
  if (address_.valid())
    return SendRequestToHost(address_, path, request, response);

  int pos = hostname_.find_last_of(':');
  if (pos < 0)
    throw runtime_error(StringPrintf("Invalid host:port %s", hostname_.c_str()));

  vector<Socket::Address> addrs = Socket::Resolve(hostname_.substr(0, pos).c_str());
  if (!addrs.size())
    throw runtime_error(StringPrintf("No addresses found for %s", hostname_.c_str()));

  BOOST_FOREACH(Socket::Address &addr, addrs) {
    addr.set_port(lexical_cast<uint16_t>(hostname_.substr(pos + 1)));
    try {
      return SendRequestToHost(addr, path, request, response);
    } catch (runtime_error) {
      LOG(WARNING) << "Connection to " << addr.ToString() << " failed";
      continue;
    }
  }
  throw runtime_error(StringPrintf("Unable to connect to %s", hostname_.c_str()));
}

proto::AddResponse *StoreClient::Add(const proto::AddRequest &req) {
  scoped_ptr<proto::AddResponse> response(new proto::AddResponse());
  SendRequest("/add", req, response.get());
  if (!response->success())
    throw runtime_error(StringPrintf("Server Error: %s", response->errormessage().c_str()));
  return response.release();
}

proto::ListResponse *StoreClient::List(const proto::ListRequest &req) {
  if (store_config_) {
    // Send request to all storage servers
    vector<proto::ListResponse *> responses;
    BackgroundExecutor executor;
    for (int i = 0; i < store_config_->config().server_size(); i++) {
      const proto::StoreServer &server = store_config_->config().server(i);
      proto::ListResponse *response = new proto::ListResponse();
      responses.push_back(response);
      executor.Add(bind(&StoreClient::SendRequestToHost, this, Socket::Address(server.address()), "/list", req,
                        response));
    }
    executor.JoinThreads();

    scoped_ptr<proto::ListResponse> output(new proto::ListResponse());
    output->set_success(false);
    output->set_errormessage("No responses");
    for (vector<proto::ListResponse *>::iterator i = responses.begin(); i != responses.end(); ++i) {
      proto::ListResponse *response = *i;
      if (response->success()) {
        output->set_success(true);
      } else {
        output->set_errormessage(response->errormessage());
      }
      // Copy streams from response
      for (int j = 0; j < response->stream_size(); j++) {
        proto::ValueStream *stream = output->add_stream();
        stream->CopyFrom(response->stream(j));
      }
      delete response;
      if (output->success())
        output->clear_errormessage();
    }
    return output.release();
  } else {
    // Send request to a single storage server
    scoped_ptr<proto::ListResponse> response(new proto::ListResponse());
    SendRequest("/list", req, response.get());
    if (!response->success())
      throw runtime_error(StringPrintf("Server Error: %s", response->errormessage().c_str()));
    return response.release();
  }
}

proto::GetResponse *StoreClient::Get(const proto::GetRequest &req) {
  if (store_config_) {
    // Send request to all storage servers
    vector<proto::GetResponse *> responses;
    BackgroundExecutor executor;
    for (int i = 0; i < store_config_->config().server_size(); i++) {
      const proto::StoreServer &server = store_config_->config().server(i);
      proto::GetResponse *response = new proto::GetResponse();
      responses.push_back(response);
      executor.Add(bind(&StoreClient::SendRequestToHost, this, Socket::Address(server.address()), "/get", req,
                        response));
    }
    executor.JoinThreads();

    scoped_ptr<proto::GetResponse> output(new proto::GetResponse());
    output->set_success(false);
    output->set_errormessage("No responses");
    for (vector<proto::GetResponse *>::iterator i = responses.begin(); i != responses.end(); ++i) {
      proto::GetResponse *response = *i;
      if (response->success()) {
        output->set_success(true);
      } else {
        output->set_errormessage(response->errormessage());
      }
      // Copy streams from response
      for (int j = 0; j < response->stream_size(); j++) {
        proto::ValueStream *stream = output->add_stream();
        stream->CopyFrom(response->stream(j));
      }
      delete response;
      if (output->success())
        output->clear_errormessage();
    }
    return output.release();

  } else {
    // Send request to a single storage server
    scoped_ptr<proto::GetResponse> response(new proto::GetResponse());
    SendRequest("/get", req, response.get());
    if (!response->success())
      throw runtime_error(StringPrintf("Server Error: %s", response->errormessage().c_str()));
    return response.release();
  }
}

proto::StoreConfig *StoreClient::GetStoreConfig() {
  scoped_ptr<proto::StoreConfig> response(new proto::StoreConfig());
  SendRequest("/get_config", *response, response.get());
  return response.release();
}

}  // namespace openinstrument
