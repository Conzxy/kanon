#ifndef KANON_NET_CALLBACK_H
#define KANON_NET_CALLBACK_H

#include "kanon/time/TimeStamp.h"

#include <memory>
#include <functional>

namespace kanon {

class TcpConnection;
class Buffer;

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef std::function<void(TcpConnectionPtr const&)> ConnectionCallback;
typedef std::function<void(TcpConnectionPtr const&)> WriteCompleteCallback;
typedef std::function<void(TcpConnectionPtr const&, size_t)> HighWaterMarkCallback;
typedef std::function<void(TcpConnectionPtr const&)> CloseCallback;

typedef std::function<void(TcpConnectionPtr const&, Buffer&, TimeStamp stamp)> MessageCallback;


} // namespace kanon

#endif // KANON_NET_CALLBACK_H
