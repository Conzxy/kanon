#include "kanon/net/connection/unix_connection.h"

using namespace std;

namespace kanon {

UnixConnection::UnixConnection(EventLoop* loop, string const& name, int sockfd)
  : Base(loop, name, sockfd)
{
  LOG_TRACE_KANON << "UnixConnection [" << name << "]" << "is created";
}

UnixConnection::~UnixConnection() noexcept {
  LOG_TRACE_KANON << "UnixConnection [" << name_ << "]" << "is destroyed";
}

} // namespace kanon
