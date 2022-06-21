#ifndef KANON_NET_CALLBACK_H
#define KANON_NET_CALLBACK_H

#include <memory>
#include <functional>

#include "kanon/util/time_stamp.h"

namespace kanon {

class TcpConnection;

namespace buffer {
class LinearBuffer;
}

using Buffer                = buffer::LinearBuffer;
using TcpConnectionPtr      = std::shared_ptr<TcpConnection>;
using ConnectionCallback    = std::function<void(TcpConnectionPtr const&)>;
using WriteCompleteCallback = std::function<bool(TcpConnectionPtr const&)>;
using HighWaterMarkCallback = std::function<void(TcpConnectionPtr const&, size_t)>;
using CloseCallback         = std::function<void(TcpConnectionPtr const&)>;
using TimerCallback         = std::function<void()>;
using MessageCallback       = std::function<void(TcpConnectionPtr const&, buffer::LinearBuffer&, TimeStamp stamp)>;

// The maximum number of used parameter is 3
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

} // namespace kanon

#endif // KANON_NET_CALLBACK_H
