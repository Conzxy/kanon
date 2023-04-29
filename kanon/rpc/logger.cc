#include "kanon/rpc/logger.h"

using namespace kanon;

bool kanon::detail::g_kanon_protobuf_rpc_log_enable = true;

static int InitKanonProtobufRpcLogEnable() KANON_NOEXCEPT
{
  auto enable = ::getenv("KANON_PROTOBUF_RPC_LOG_ENABLE");
  if (enable && !strcmp(enable, "0")) {
    kanon::detail::g_kanon_protobuf_rpc_log_enable = false;
  }
  return 0;
}

static int dummy_kanon_protobuf_rpc_log_enable_register =
    InitKanonProtobufRpcLogEnable();
