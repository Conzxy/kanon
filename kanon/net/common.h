#ifndef KANON_NET_COMMON_H
#define KANON_NET_COMMON_H

// The file is just collection of some header
// that are very likely included by user
#include "kanon/net/EventLoop.h"
#include "kanon/net/TcpConnection.h"
#include "kanon/net/Buffer.h"
#include "kanon/net/InetAddr.h"

using kanon::EventLoop;
using kanon::TcpConnectionPtr;
using kanon::Buffer;
using kanon::InetAddr;
using kanon::noncopyable;
using kanon::TimeStamp;
using kanon::StringView;

#endif // KANON_NET_COMMON_H
