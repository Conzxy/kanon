#include "codec.h"

#include "kanon/net/buffer.h"
#include "kanon/net/connection/tcp_connection.h"

using namespace kanon;

constexpr int LengthHeaderCodec::kHeaderLength;

LengthHeaderCodec::LengthHeaderCodec() = default;

LengthHeaderCodec::LengthHeaderCodec(MessageCallback cb)
  : message_callback_{ std::move(cb) }
{
}

void LengthHeaderCodec::Send(TcpConnectionPtr const& conn, StringView msg)
{
  Buffer output_buffer;
  
  uint32_t length = msg.size(); 
  output_buffer.Append(msg);
  output_buffer.Prepend32(length);

  LOG_DEBUG << "length = " << length; 
  LOG_DEBUG << "length header = " << output_buffer.GetReadBegin32();
  conn->Send(output_buffer);
}

void LengthHeaderCodec::Send(TcpConnectionPtr const& conn, OutputBuffer& buffer) {
  uint32_t len = buffer.GetReadableSize();

  LOG_DEBUG << "length = " << len;
  buffer.Prepend32(len);
  assert(buffer.Read32() == len);
  conn->Send(buffer);
}

void LengthHeaderCodec::OnMessage(TcpConnectionPtr const& conn,
                                  Buffer& buf,
                                  TimeStamp receive_time)
{
  // Since tcp is stream protocol,
  // we should use while instread of just a if
  while (buf.GetReadableSize() >= kHeaderLength) {
    uint32_t message_len = buf.GetReadBegin32();
    
    LOG_DEBUG << "message_len: " << message_len;
    // Check length if valid
    LOG_DEBUG << "buf readable size = " << buf.GetReadableSize();
    LOG_DEBUG << "content: " << buf.ToStringView();
    if (message_len > max_accept_len_) {
      LOG_ERROR << "Invalid message length";
      conn->ShutdownWrite();
      break; 
      
    } else if (buf.GetReadableSize() >= (kHeaderLength + message_len)) {
      buf.AdvanceRead(kHeaderLength);

      message_callback_(conn, buf, receive_time);
    } else {
      // if not a complete packet, just exit loop.
      LOG_DEBUG << "Not a complete message";
      break;
    }
  }
}
