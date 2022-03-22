#include "chat_server.h"

void ChatServer::OnConnection(TcpConnectionPtr const& conn) {
  if (conn->IsConnected()) {
    conns_.insert(conns_.end(), conn);
  } else {
    conns_.erase(conn);
  }
}

void ChatServer::OnStringMessage(
    TcpConnectionPtr const& conn,
    std::string const&  msg,
    TimeStamp receive_time)
{
  // BroadCast to every user
  //
  KANON_UNUSED(conn);
  KANON_UNUSED(receive_time);
  for (auto const& c : conns_) {
    codec_.Send(c, msg);
  }
}
