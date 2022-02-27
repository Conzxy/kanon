#ifndef KANON_NET_USER_CLIENT_H
#define KANON_NET_USER_CLIENT_H

// The file is just a collection of some header
// that client most likely include
#include "kanon/net/tcp_client.h"
#include "kanon/net/inet_addr.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/event_loop_thread.h"
#include "kanon/net/buffer.h"
#include "kanon/net/tcp_connection.h"

using kanon::EventLoop;
using kanon::EventLoopThread;
using kanon::TcpConnectionPtr;
using kanon::Buffer;
using kanon::InetAddr;
using kanon::TimeStamp;
using kanon::StringView;
using kanon::TcpClient;

#endif // KANON_NET_USER_CLIENT_H