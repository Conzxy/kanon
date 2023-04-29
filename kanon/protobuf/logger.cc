#include "kanon/protobuf/logger.h"

using namespace kanon;

bool kanon::detail::g_kanon_protobuf_log = true;

static int InitKanonProtobufLogEnable() KANON_NOEXCEPT
{
  auto enable = ::getenv("KANON_PROTOBUF_LOG_ENABLE");
  if (enable && !strcmp(enable, "0")) {
    kanon::detail::g_kanon_protobuf_log = false;
  }
  return 0;
}

static int dummy_kanon_protobuf_log_enable_register =
    InitKanonProtobufLogEnable();
