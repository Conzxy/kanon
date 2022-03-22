#ifndef KANON_CHAT_CLIEANT_H
#define KANON_CHAT_CLIEANT_H

#include "kanon/net/user_client.h"
#include "kanon/thread/count_down_latch.h"

#include "example/length_codec/codec.h"

class ChatClient : TcpClient {
public:
  explicit ChatClient(EventLoop* loop, InetAddr const& server_addr);

  void OnStringMessage(TcpConnectionPtr const& conn,
    std::string const& msg,
    TimeStamp receive_time);
  
  void OnConnection(TcpConnectionPtr const& conn); 

  void WriteMessage(kanon::StringView msg);

  void Connect();
  void Disconnect();
private:
  kanon::LengthHeaderCodec codec_;
  kanon::MutexLock mutex_;
  TcpConnectionPtr conn_;
  
  kanon::CountDownLatch latch_;
};

#endif // KANON_CHAT_CLIEANT_H
