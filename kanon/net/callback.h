#ifndef KANON_NET_CALLBACK_H
#define KANON_NET_CALLBACK_H

#include "kanon/time/time_stamp.h"

#include <memory>
#include <functional>

namespace kanon {

class TcpConnection;
class Buffer;

using TcpConnectionPtr      = std::shared_ptr<TcpConnection>;
using ConnectionCallback    = std::function<void(TcpConnectionPtr const&)>;
using WriteCompleteCallback = std::function<void(TcpConnectionPtr const&)>;
using HighWaterMarkCallback = std::function<void(TcpConnectionPtr const&, size_t)>;
using CloseCallback         = std::function<void(TcpConnectionPtr const&)>;
using TimerCallback         = std::function<void()>;
using MessageCallback       = std::function<void(TcpConnectionPtr const&, Buffer&, TimeStamp stamp)>;

// The maximum number of used parameter is 3
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

} // namespace kanon

#endif // KANON_NET_CALLBACK_H
