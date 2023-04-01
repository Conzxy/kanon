#ifndef KANON_RPC_CONTROLLER_H__
#define KANON_RPC_CONTROLLER_H__

#include <google/protobuf/service.h>

#include "kanon/net/connection/tcp_connection.h"
#include "kanon/util/macro.h"
#include "rpc_codec.h"

namespace kanon {
namespace protobuf {
namespace rpc {

class RpcMessage;

#define INVALID_DEADLINE ((size_t)-1)

class RpcController : public ::google::protobuf::RpcController {
 public:
  /* kanon support double(seconds), ms(integer), us(integer) */
  using Deadline = uint64_t;

  RpcController();

  virtual ~RpcController() KANON_NOEXCEPT KANON_OVERRIDE;

  /*--------------------*/
  /* Client side        */
  /*--------------------*/

  void Reset() final;

  bool Failed() const final;

  std::string ErrorText() const final;

  void StartCancel() final;

  KANON_INLINE void SetTimeout(uint64_t timeout) KANON_NOEXCEPT
  {
    deadline_ = TimeStamp::Now().GetMilliseconds() + timeout;
  }

  KANON_INLINE void SetDeadline(TimeStamp dead_line) KANON_NOEXCEPT
  {
    deadline_ = dead_line.GetMilliseconds();
  }

  KANON_INLINE void SetDeadline(uint64_t dead_line) KANON_NOEXCEPT
  {
    deadline_ = dead_line;
  }

  KANON_INLINE Deadline deadline() const KANON_NOEXCEPT
  {
    return deadline_;
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
  Deadline deadline_;
};
} // namespace rpc
} // namespace protobuf
} // namespace kanon
#endif // KANON_RPC_CONTROLLER_H__
