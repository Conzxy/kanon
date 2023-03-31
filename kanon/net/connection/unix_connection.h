#ifndef KANON_NET_UNIX_CONNECTION_H
#define KANON_NET_UNIX_CONNECTION_H

#include "connection_base.h"

namespace kanon {

class UnixConnection : public ConnectionBase<UnixConnection> {
  using Base = ConnectionBase<UnixConnection>;

 public:
  KANON_NET_NO_API UnixConnection(EventLoop *loop, std::string const &name,
                                  int sockfd);
  KANON_NET_API ~UnixConnection() KANON_NOEXCEPT;
};

} // namespace kanon

#endif // KANON_NET_UNIX_CONNECTION_H