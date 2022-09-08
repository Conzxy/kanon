#ifndef KANON_RPC_CONTROLLER_H__
#define KANON_RPC_CONTROLLER_H__

#include <google/protobuf/service.h>

#include "kanon/net/connection/tcp_connection.h"
#include "rpc_codec.h"

namespace kanon {
namespace protobuf {
namespace rpc {

class RpcMessage;

#define INVALID_TIMEOUT (size_t)-1

class RpcController : public ::google::protobuf::RpcController {
 public:
  /* kanon support double(seconds), ms(integer), us(integer) */
  using Timeout = uint64_t;

  RpcController();

  virtual ~RpcController() noexcept override;

  /*--------------------*/
  /* Client side        */
  /*--------------------*/

  void Reset() final;

  bool Failed() const final;

  std::string ErrorText() const final;

  void StartCancel() final;

  void SetTimeout(Timeout timeout);
  Timeout timeout() const noexcept
  {
    return timeout_;
  }

  /*--------------------*/
  /* Server side        */
  /*--------------------*/

  void SetFailed(std::string const &reason) final;

  bool IsCanceled() const final;

  void NotifyOnCancel(::google::protobuf::Closure *callback) final;

  /*-------------------*/
  /* Common            */
  /*-------------------*/

 private:
  Timeout timeout_;
};
} // namespace rpc
} // namespace protobuf
} // namespace kanon
#endif // KANON_RPC_CONTROLLER_H__
