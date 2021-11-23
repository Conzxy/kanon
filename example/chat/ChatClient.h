#ifndef KANON_CHAT_CLIEANT_H
#define KANON_CHAT_CLIEANT_H

#include "kanon/net/TcpClient.h"
#include "kanon/net/common.h"
#include "kanon/thread/CountDownLatch.h"

#include "../codec.h"

class ChatClient {
public:
  explicit ChatClient(EventLoop* loop,
      kanon::InetAddr const& server_addr);

  void onStringMessage(TcpConnectionPtr const& conn,
    std::string const& msg,
    TimeStamp receive_time);
  
  void onConnection(TcpConnectionPtr const& conn); 

  void writeMessage(kanon::StringView msg);

  void connect();
  void disconnect();
private:
  kanon::TcpClient client_;
  kanon::LengthHeaderCodec codec_;
  kanon::MutexLock mutex_;
  TcpConnectionPtr conn_;
  
  kanon::CountDownLatch latch_;
};

#endif // KANON_CHAT_CLIEANT_H
