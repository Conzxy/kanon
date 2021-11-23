#include "ChatServer.h"

void
ChatServer::onConnection(TcpConnectionPtr const& conn) {
  kanon::defaultConnectionCallaback(conn);
  
  // UP
  if (conn->isConnected()) {
    conns_.insert(conns_.end(), conn);
  } else { // DOWN
    conns_.erase(conn);
  }
}

void
ChatServer::onStringMessage(TcpConnectionPtr const& conn,
    std::string const&  msg,
    TimeStamp receive_time) {
  // broadcast to every connection 
  //
  // message has parsed and resides in @param msg
  // Here, just forward to others.
  //
  KANON_UNUSED(conn);
  KANON_UNUSED(receive_time);
  for (auto const& c : conns_) {
    codec_.send(c, msg);
  }
}

void
ChatServer::start() {
  // boot server and start loop
  server_.start();
  
  loop_.loop();
  //server_.setLoopNum(10);
}
