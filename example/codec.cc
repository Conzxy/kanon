#include "codec.h"

#include "kanon/net/Buffer.h"
#include "kanon/net/TcpConnection.h"

using namespace kanon;

constexpr int LengthHeaderCodec::kHeaderLength;

LengthHeaderCodec::LengthHeaderCodec(StringMessageCallback cb)
  : messageCallback_{ std::move(cb) }
{
}

void
LengthHeaderCodec::send(TcpConnectionPtr const& conn,
  StringView msg) {
  Buffer output_buffer;
  
  uint32_t length = msg.size(); 
  output_buffer.append(msg);
  output_buffer.prepend32(sock::toNetworkByteOrder32(length));
  
  LOG_DEBUG << sock::toHostByteOrder32(output_buffer.peek32()); 
  conn->send(output_buffer);
}

void
LengthHeaderCodec::onMessage(TcpConnectionPtr const& conn,
  Buffer& buf,
  TimeStamp receive_time) {
  
  // Since tcp is stream protocol,
  // we should use while instread of just a if
  while (buf.readable_size() >= kHeaderLength) {
    uint32_t message_len = sock::toHostByteOrder32(buf.peek32());
    
    LOG_DEBUG << "message_len: " << message_len;
    // Check length if valid
    if (message_len > 65536 || message_len < 0) {
      LOG_ERROR << "Invalid message length";
      conn->shutdownWrite();
      break; 
      
      // if not a complete packet, just exit loop. 
    } else if (buf.readable_size() >= kHeaderLength + message_len) {
      buf.advance(kHeaderLength);
      const auto msg = buf.retrieveAsString(message_len);
      messageCallback_(conn, msg, receive_time);
    } else {
      break;
    }
  }
}
