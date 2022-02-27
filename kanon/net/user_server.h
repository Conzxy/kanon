#ifndef KANON_NET_USER_SERVER_H
#define KANON_NET_USER_SERVER_H

// The file is just a collection of some header
// that server most likely include
#include "kanon/net/tcp_server.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/event_loop_thread.h"
#include "kanon/net/tcp_connection.h"
#include "kanon/net/buffer.h"
#include "kanon/net/inet_addr.h"

using kanon::EventLoop;
using kanon::EventLoopThread;
using kanon::TcpConnectionPtr;
using kanon::Buffer;
using kanon::InetAddr;
using kanon::TimeStamp;
using kanon::StringView;
using kanon::TcpServer;

#endif // KANON_NET_USER_SERVER_H