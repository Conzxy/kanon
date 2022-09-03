#ifndef KANON_KRPC_RPCHANNLE_H_
#define KANON_KRPC_RPCHANNLE_H_

#include <atomic>
#include <unordered_map>

// ProtobufCodec<> need class definition for following reasons:
// 1. std::is_base_of<>
// 2. constructor of ConcreMessage
#include "kanon/net/callback.h"
#include "kanon/protobuf/protobuf_codec.h"
#include "kanon/rpc/rpc.pb.h"
#include "kanon/thread/mutex_lock.h"
#include "kanon/util/noncopyable.h"
#include "callable.h"

#include <google/protobuf/service.h>

namespace kanon {
namespace protobuf {
namespace rpc {

extern char const krpc_tag[];

/**
 *
 */
class RpcChannel
  : public noncopyable
  , public ::google::protobuf::RpcChannel {
  using Codec = ProtobufCodec<RpcMessage, krpc_tag>;
  using ErrorCode = RpcMessage::ErrorCode;
  // using RpcMessagePtr = std::unique_ptr<RpcMessage>;
  using RpcMessagePtr = RpcMessage *;

 public:
  using ServiceMap = std::unordered_map<std::string, PROTOBUF::Service *>;

  /** Used for client */
  RpcChannel();
  /** Used for server */
  explicit RpcChannel(TcpConnectionPtr const &conn);
  ~RpcChannel();

  void SetConnection(TcpConnectionPtr const &conn) noexcept;
  /** Used for server */
  void SetServices(ServiceMap const &services) noexcept
  {
    services_ = &services;
  }

  /**
   * \param request the lifetime is managed by user
   * \param response the lifetime is managed by user(common, delete in the done
   * callback) \note Called by Stub(RpcChannel wrapper) i.e. client
   */
  void CallMethod(PROTOBUF::MethodDescriptor const *method,
                  PROTOBUF::RpcController *controller,
                  PROTOBUF::Message const *request, PROTOBUF::Message *response,
                  PROTOBUF::Closure *done) override;

 private:
  /**
   * Wrapper of the callback that running in the loop
   * since c++11 don't allow variable initialization in capture list
   */
  void SendRpcRequest(RpcMessage const &message, PROTOBUF::Message *response,
                      PROTOBUF::Closure *done);
  /**
   * This call the Service::CallMethod(), the logic is
   * fill the response according to the request
   * defined in the derived class of Service.
   *
   * The lifetime of request is managed by user.
   * The lifetime of response is managed by SendRpcResponse().
   *
   * This allow user fill request and call SendRpcResponse()
   * in the other thread
   */
  void OnRpcMessageForRequest(TcpConnectionPtr const &conn,
                              RpcMessagePtr message, TimeStamp stamp);

  void OnRpcMessageForResponse(TcpConnectionPtr const &conn,
                               RpcMessagePtr message, TimeStamp stamp);

  void OnRpcMessage(TcpConnectionPtr const &conn, RpcMessagePtr message,
                    TimeStamp stamp);

  /**
   * Fill error message and send
   */
  void ErrorHandle(uint64_t id, ErrorCode error_code);

  /**
   * Fill response and send
   * Used for server side
   */
  void SendRpcResponse(PROTOBUF::Message *response, uint64_t id);

 private:
  TcpConnectionPtr conn_;
  std::atomic<uint64_t> id_;
  Codec codec_;

  /**
   * xxxServiceImpl is a concrete class, can use raw pointer
   * According the service string from the request get the specific service
   * then call Service::CallMethod()
   * ! Used for server side
   */
  ServiceMap const *services_;

  /**
   * response and done are setted by client
   *
   * Store response since rpc message is the internal
   * message, we must parse it in the OnMessage handler.
   *
   * Store the done since this is a asynchronous callback
   *
   */
  struct CallResult {
    PROTOBUF::Message *response;
    PROTOBUF::Closure *done;
  };

  /**
   * CallResult is a pair of <response, done>
   * response and done is filled by client
   * ! Used for client side
   */
  std::unordered_map<int, CallResult>
      call_results_; // GUARDED_BY call_result_lock_
};

} // namespace rpc
} // namespace protobuf
} // namespace kanon

#endif // KANON_KRPC_RPCHANNLE_H_
