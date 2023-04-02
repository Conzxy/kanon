#ifndef KANON_PROTOBUF_RPC_LOGGER_H__
#define KANON_PROTOBUF_RPC_LOGGER_H__

#include "kanon/log/logger.h"

namespace kanon {
namespace detail {
extern bool g_kanon_protobuf_rpc_log_enable;
} // namespace detail
} // namespace kanon

#define LOG_TRACE_KANON_PROTOBUF_RPC                                           \
  if (::kanon::detail::g_kanon_protobuf_rpc_log_enable) LOG_TRACE

#define LOG_DEBUG_KANON_PROTOBUF_RPC                                           \
  if (::kanon::detail::g_kanon_protobuf_rpc_log_enable) LOG_DEBUG

#define LOG_INFO_KANON_PROTOBUF_RPC                                            \
  if (::kanon::detail::g_kanon_protobuf_rpc_log_enable) LOG_INFO

#define LOG_WARN_KANON_PROTOBUF_RPC                                            \
  if (::kanon::detail::g_kanon_protobuf_rpc_log_enable) LOG_WARN

#define LOG_ERROR_KANON_PROTOBUF_RPC                                           \
  if (::kanon::detail::g_kanon_protobuf_rpc_log_enable) LOG_ERROR

#define LOG_SYSERROR_KANON_PROTOBUF_RPC                                        \
  if (::kanon::detail::g_kanon_protobuf_rpc_log_enable) LOG_SYSERROR

#endif
