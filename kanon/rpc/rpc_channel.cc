#include <functional>

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>

#include "kanon/log/logger.h"
#include "kanon/net/connection/tcp_connection.h"
#include "kanon/net/event_loop.h"
#include "kanon/util/macro.h"

#include "rpc_channel.h"
#include "rpc_controller.h"

using PROTOBUF::Closure;
using PROTOBUF::Message;
using PROTOBUF::MethodDescriptor;
using PROTOBUF::Service;

using namespace kanon::protobuf::rpc;

static char const *GetRpcErrorString(RpcMessage::ErrorCode error) noexcept;

RpcChannel::RpcChannel() = default;

// Constructor does not recommended to do many thing except for initial work
RpcChannel::RpcChannel(TcpConnectionPtr const &conn)
  : id_(0)
  , codec_()
{
  SetConnection(conn);
}

RpcChannel::~RpcChannel() = default;

void RpcChannel::ErrorHandle(uint64_t id, ErrorCode error_code)
{
  RpcMessage message;
  message.set_id(id);
  message.set_type(RpcMessage::kResponse);
  message.set_error(static_cast<RpcMessage::ErrorCode>(error_code));
  codec_.Send(conn_, &message);
}

void RpcChannel::CallMethod(MethodDescriptor const *method,
                            PROTOBUF::RpcController *controller,
                            Message const *request, Message *response,
                            Closure *done)
{
  RpcController *contr = kanon::down_pointer_cast<RpcController>(controller);

  RpcMessage message;
  auto id = id_.load(std::memory_order_relaxed);
  message.set_id(id);
  id_.fetch_add(1, std::memory_order_relaxed);

  message.set_type(RpcMessage::kRequest);
  message.set_request(request->SerializeAsString());

  // FIXME fullname()?
  message.set_method(method->name());
  message.set_service(method->service()->full_name());

  conn_->GetLoop()->RunInLoop(std::bind(&RpcChannel::SendRpcRequest, this,
                                        std::move(message), response, done,
                                        contr));
}

void RpcChannel::SendRpcRequest(RpcMessage &message, Message *response,
                                Closure *done, RpcController *controller)
{
  // Must insert <id, outstanding_call> into outstanding_calls_ first
  // There are maybe server response reach but outstanding_call is not
  // registered.
  //
  // O(1) insert since id increase response and done is provided
  // by client
  //
  // If done is NULL, indicates the user don't expect response
  // i.e. void return procedure
  if (done) {
    // outstanding_calls_.emplace_hint(outstanding_calls_.end(), message.id(),
    //                                 OutstandingCall{response, done});
    outstanding_calls_.emplace(message.id(), OutstandingCall{response, done});
  } else {
    KANON_ASSERT(!response,
                 "Notifiction(done is NULL) no need to set response");
  }

  if (controller->deadline() != INVALID_DEADLINE)
    message.set_deadline(controller->deadline());

  codec_.Send(conn_, &message);

  if (controller->deadline() != INVALID_DEADLINE) {
    auto id = message.id();
    conn_->GetLoop()->RunAt(
        [this, id]() {
          outstanding_calls_.erase(id);
        },
        TimeStamp((int64_t)controller->deadline() * 1000));
  }
}

void RpcChannel::SendRpcResponse(Message *response, uint64_t id)
{
  // The method is completed, fill RpcMessae and send
  RpcMessage message;
  message.set_id(id);
  message.set_response(response->SerializeAsString());
  message.set_type(RpcMessage::kResponse);

  codec_.Send(conn_, &message);
  // The response is new by Service::GetResponsePrototype().
  // Must delete it to avoid memory leak.
  delete response;
}

static char const *GetRpcErrorString(RpcMessage::ErrorCode error) noexcept
{
  switch (error) {
    case RpcMessage::kNoError:
      return "No Error";
    case RpcMessage::kNoMethod:
      return "No Method";
    case RpcMessage::kNoService:
      return "No Service";
    case RpcMessage::kInvalidMessage:
      return "Invalid Message";
    case RpcMessage::kInvalidRequest:
      return "Invalid Request";
    case RpcMessage::kInvalidResponse:
      return "Invalid Response";
    default:
      return "Unknown Error";
  }
}

void RpcChannel::OnRpcMessage(TcpConnectionPtr const &conn,
                              RpcMessagePtr message, TimeStamp receive_time)
{
  KANON_ASSERT(conn == conn_,
               "The connection from RpcMessage on_message callback must be "
               "same with the connnection in RpcChannel");
  KANON_UNUSED(conn);

  LOG_DEBUG_KANON << "RpcMessage(Not raw message) receive_time: "
                  << receive_time.ToFormattedString();

  KANON_ASSERT(message->has_type() && message->has_id(),
               "The type and id field is required, the protobuf of peer must "
               "ensure them are setted");

  if (message->type() == RpcMessage::kResponse) {
    OnRpcMessageForResponse(conn, message, receive_time);
  } else if (message->type() == RpcMessage::kRequest) {
    OnRpcMessageForRequest(conn, message, receive_time);
  } else {
    KANON_ASSERT(false, "RpcMessage Type Error occurred impossible, it is must "
                        "be internal error");
  }
}

void RpcChannel::OnRpcMessageForResponse(TcpConnectionPtr const &conn,
                                         RpcMessagePtr message, TimeStamp stamp)
{
  /**
   * Receive the response from the RpcServer
   * 0. Check error, response
   * 0.1. If error is setted, log error and abort
   * 1. Get the response and done callback from the outstanding_calls_ and
   * remove it
   * 2. Fill(/Parse) the response
   * 3. Call done with the response, it will process the response
   */
  if (!message->has_error()) {
    // The response must be setted in RpcServer
    KANON_ASSERT(message->has_response(),
                 "Server Internal error or unknown error");

    const uint64_t id = message->id();

    KANON_ASSERT(id <= id_, "The id in response should be not greater than the "
                            "id_ maintained by client");

    // Thread safe since the CallMethod()
    // insert the outstanding_calls in the loop.
    OutstandingCall outstanding_call{nullptr, nullptr};

    auto it = outstanding_calls_.find(id);

    // KANON_ASSERT(it != outstanding_calls_.end(),
    //              "The outstanding_call must be setted before the RpcMessage "
    //              "on_message Callback");

    // If deadline is setted, may the done is erased
    if (it == outstanding_calls_.end()) {
      return;
    }
    outstanding_call = it->second;
    outstanding_calls_.erase(it);

    // done and response is setted by client
    KANON_ASSERT(outstanding_call.response != nullptr,
                 "response must be setted by client");
    KANON_ASSERT(outstanding_call.done != nullptr,
                 "done must be setted by client");

    // done->Run() will delete this itself
    // i.e. self-delete
    // std::unique_ptr<Closure> done_wrapper(outstanding_call.done);

    const auto res =
        outstanding_call.response->ParseFromString(message->response());
    KANON_UNUSED(res);

    KANON_ASSERT(
        res, "There are some error occurred in the parse of  response contents"
             "Server error or probobuf internal error");

    // done manage the lifetime of response
    outstanding_call.done->Run();
  } else {
    // Response with error setted
    const auto n = outstanding_calls_.erase(message->id());
    KANON_UNUSED(n);
    assert(n == 1);

    // Since this error is logic error(i.e. ensure it don't happened)
    LOG_FATAL << "Rpc error message from server: "
              << GetRpcErrorString(message->error());
  }
}

void RpcChannel::OnRpcMessageForRequest(TcpConnectionPtr const &conn,
                                        RpcMessagePtr message, TimeStamp stamp)
{
  /**
   * Receive the request message from RpcClient
   * 1. Check the service
   * 1.1 If service is not setted, set error_code
   * 2. Find the service and use it get the service descriptor
   * 3. Check the method
   * 3.1 If method is not setted, set error_code
   * 4. Find the method then get the method descriptor
   * 5. Get the request and the response of the method
   * 6. Parse the request and call the Service::CallMethod(), it will call
   *    the specific method to process request and fill the response
   * 7. Send the response(By the "done" argument of Service::CallMethod())
   *
   * https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.service?hl=en
   * Example:
   * const MethodDescriptor* method =
   *   service->GetDescriptor()->FindMethodByName("Foo");
   * Message* request  = stub->GetRequestPrototype (method)->New();
   * Message* response = stub->GetResponsePrototype(method)->New();
   * request->ParseFromString(input);
   * service->CallMethod(method, *request, response, callback);
   */
  RpcMessage::ErrorCode error_code = RpcMessage::kNoError;
  if (message->has_request() && message->has_service() && message->has_method())
  {
    const auto it = services_->find(message->service());

    if (it != services_->end()) {
      Service *service = it->second;
      MethodDescriptor const *method =
          service->GetDescriptor()->FindMethodByName(message->method());

      if (method) {
        // request only used in specific method
        // but the response will used in "done" callback
        // then free it in SendRpcResponse
        auto request = service->GetRequestPrototype(method).New();

        // response's lifetime managed by SendRpcResponse()
        // prototype pattern
        Message *response = service->GetResponsePrototype(method).New();

        if (request->ParseFromString(message->request())) {
          // Closure see google/protobuf/stubs/callback.h
          // NewCallback() support one or two argument
          // NewCallback() accept type should be perfect match, i.e. don't allow
          // implicit cast e.g. void Foo(std::string); NewCallback(&Foo, "a");
          // // Don't works
          //
          // The Service::CallMethod is a virtual function,
          // will call the named overrided function in the concrete
          // derived class of Service.
          //
          // The request need managed by concrete service function
          RpcController *controller = new RpcController;
          if (message->has_deadline()) {
            controller->SetDeadline(message->deadline());
            LOG_DEBUG << "deadline=" << controller->deadline() << " Ms";
          }
          service->CallMethod(
              method, controller, request, response,
              PROTOBUF::NewCallback(this, &RpcChannel::SendRpcResponse,
                                    response, message->id()));
        } else {
          error_code = RpcMessage::kInvalidRequest;
        }
      } else {
        error_code = RpcMessage::kNoMethod;
      }

    } else {
      error_code = RpcMessage::kNoService;
    }
  } else {
    // The Message indicates the rpc message wrapper(warp request)
    // instead of the request (message) field.
    error_code = RpcMessage::kInvalidMessage;
  }

  if (error_code != RpcMessage::kNoError) {
    ErrorHandle(message->id(), error_code);
  }
}

void RpcChannel::SetConnection(const TcpConnectionPtr &conn) noexcept
{
  conn_ = conn;

  // Forward to codec to process raw-format message
  conn_->SetMessageCallback(std::bind(&Codec::OnMessage, &codec_, _1, _2, _3));

  // Handle the protobuf-format RpcMessage(i.e. the payload after the codec_
  // parsing)
  codec_.SetMessageCallback(
      std::bind(&RpcChannel::OnRpcMessage, this, _1, _2, _3));
}
