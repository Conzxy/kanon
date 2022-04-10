#ifndef KANON_CODEC_H
#define KANON_CODEC_H

#include "kanon/util/noncopyable.h"
#include "kanon/net/callback.h"
#include "kanon/string/string_view.h"

namespace kanon {

/**
 * Parse the length header in message stream(boundaryless)
 * to get a complete message packet(fixed length).
 */
class LengthHeaderCodec : noncopyable {
public:
  // using StringMessageCallback =
  //   std::function<void(TcpConnectionPtr const&, std::string const&, TimeStamp)>;

  using MessageCallback = 
    std::function<void(TcpConnectionPtr const&, std::vector<char>&, TimeStamp)>;

  LengthHeaderCodec();
  explicit LengthHeaderCodec(MessageCallback cb);

  void SetMaximumAcceptLength(uint64_t len) noexcept
  { max_accept_len_ = len; }

  void Send(TcpConnectionPtr const& conn, StringView msg);
  void OnMessage(TcpConnectionPtr const& conn,
                 Buffer& buf, 
                 TimeStamp recv_time);

private:
  MessageCallback message_callback_;
  uint64_t max_accept_len_ = 65536;

  static constexpr int kHeaderLength = 4;
};

} // namespace kanon

#endif // KANON_CODEC_H
