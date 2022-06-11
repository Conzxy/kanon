#ifndef KANON_NET_UNIX_CONNECTION_H
#define KANON_NET_UNIX_CONNECTION_H

#include "connection_base.h"

namespace kanon {

class UnixConnection : public ConnectionBase<UnixConnection> {
  using Base = ConnectionBase<UnixConnection>;
public:
  UnixConnection(EventLoop* loop, std::string const& name, int sockfd);
  ~UnixConnection() noexcept;
};

} // namespace kanon

#endif // KANON_NET_UNIX_CONNECTION_H