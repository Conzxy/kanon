#ifndef KANON_CODEC_H
#define KANON_CODEC_H

#include "kanon/util/noncopyable.h"
#include "kanon/net/callback.h"
#include "kanon/string/string_view.h"

namespace kanon {

/**
 * @class LengthHeaderCodec
 * @brief 
 * parse the length header in message stream(boundaryless)
 * to get a complete message packet(fixed length).
 */
class LengthHeaderCodec : noncopyable {
public:
  typedef std::function<void(TcpConnectionPtr const&, std::string const&, TimeStamp)>
    StringMessageCallback;

  explicit LengthHeaderCodec(StringMessageCallback cb);
  
  void Send(TcpConnectionPtr const& conn, StringView msg);
  void OnMessage(TcpConnectionPtr const& conn,
      Buffer& buf, 
      TimeStamp);

private:
  static constexpr int kHeaderLength = 4;
  StringMessageCallback messageCallback_;
};

} // namespace kanon

#endif // KANON_CODEC_H
