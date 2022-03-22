#include "codec.h"

#include "kanon/net/buffer.h"
#include "kanon/net/tcp_connection.h"

using namespace kanon;

constexpr int LengthHeaderCodec::kHeaderLength;

LengthHeaderCodec::LengthHeaderCodec(StringMessageCallback cb)
  : messageCallback_{ std::move(cb) }
{
}

void
LengthHeaderCodec::Send(TcpConnectionPtr const& conn,
  StringView msg) {
  Buffer output_buffer;
  
  uint32_t length = msg.size(); 
  output_buffer.Append(msg);
  output_buffer.Prepend32(sock::ToNetworkByteOrder32(length));
  
  LOG_DEBUG << sock::ToHostByteOrder32(output_buffer.GetReadBegin32()); 
  conn->Send(output_buffer);
}

void
LengthHeaderCodec::OnMessage(TcpConnectionPtr const& conn,
  Buffer& buf,
  TimeStamp receive_time) {
  
  // Since tcp is stream protocol,
  // we should use while instread of just a if
  while (buf.GetReadableSize() >= kHeaderLength) {
    uint32_t message_len = sock::ToHostByteOrder32(buf.GetReadBegin32());
    
    LOG_DEBUG << "message_len: " << message_len;
    // Check length if valid
    if (message_len > 65536 || message_len < 0) {
      LOG_ERROR << "Invalid message length";
      conn->ShutdownWrite();
      break; 
      
      // if not a complete packet, just exit loop. 
    } else if (buf.GetReadableSize() >= kHeaderLength + message_len) {
      buf.AdvanceRead(kHeaderLength);
      const auto msg = buf.RetrieveAsString(message_len);
      messageCallback_(conn, msg, receive_time);
    } else {
      break;
    }
  }
}
