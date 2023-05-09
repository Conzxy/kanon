#ifndef _KANON_PROTOBUF_CODEC2_H__
#define _KANON_PROTOBUF_CODEC2_H__

#include <string.h>

#include <functional>
#include <memory>

#include "kanon/string/string_view.h"
#include "kanon/net/callback.h"
#include "kanon/net/endian_api.h"
#include "kanon/util/noncopyable.h"

namespace google {
namespace protobuf {

// fwd
class Message;

} // namespace protobuf
} // namespace google

namespace kanon {

class Buffer;

namespace protobuf {

// clang-format off
/**
 * Message Raw Format:
 * +---------------+
 * | size    | 4B  |
 * +---------------+ ----
 * | tag     | MB  |  |
 * +---------------+
 * | payload | NB  | size
 * +---------------+
 * | checksum| 4B  |  |
 * +---------------+ ----
 *
 * To the old ProtbufCodec(`GenericProtobufCodec`), the codec is more
 * flexible.
 * Its MessageCallback signature is `void(TcpConnectionPtr, Buffer&,
 * size_t payload_size, TimeStamp)`, the `payload_size` is the length of the
 * protobuf message.
 *
 * Provide this argument, user can get the length directly
 * other than calling `ByteSizeLong()`. 
 *
 * In most case, this is more useful than
 * the `GenericProtobufCodec` and `ProtobufCodec`. eg. Log the message to disk
 * that don't requires serialize and compute the length of message.
 * \note
 * 1. checksum compute the (tag, payload) content
 * 2. Only the payload is protobuf-format, others are just some trivial binary
 * data
 *
 * \warning
 * The class just an internal class, You should use ProtobufCodec<>
 */
// clang-format on
class ProtobufCodec2 : noncopyable {
  enum ErrorCode {
    E_NOERROR = 0,
    E_INVALID_SIZE_HEADER,
    E_INVALID_CHECKSUM,
    E_INVALID_MESSAGE,
    E_NO_COMPLETE_MESSAGE, // This is not a error, just indicator
  };

  using SizeHeaderType = uint32_t;
  using CheckSumType = uint32_t;

  static constexpr uint8_t CHECKSUM_LENGTH = sizeof(CheckSumType);
  // static constexpr uint8_t SIZE_HEADER_LENGTH = sizeof(SizeHeaderType);

 public:
  /**
   * \param conn
   * \param buffer
   * \param payload_size The payload length(ie. message). User can avoid to call
   *                     ByteSizeLong() to get the size of message.
   *                     Usage:
   *                      - Advance buffer read index
   *                      - Log to the file need size
   *                      - Parse message need size(Parse.*FromArray())
   * \param recv_time
   */
  using MessageCallback =
      std::function<void(TcpConnectionPtr const &conn, Buffer &buffer,
                         size_t payload_size, TimeStamp recv_time)>;

  /**
   * \param conn
   * \param errcode see \ref ProtobufCodec2::ErrorCode to get more information
   */
  using ErrorCallback =
      std::function<void(TcpConnectionPtr const &conn, ErrorCode errcode)>;

  /**
   * \brief Create a Protobuf message Codec
   *
   * \param tag The unique identifier to distinguish message
   * \param max_size The maximum size message can hold
   */
  ProtobufCodec2(StringView tag, size_t max_size);
  ~ProtobufCodec2() noexcept;

  /**
   * Set the message callback of the connection
   */
  void SetUpConnection(TcpConnectionPtr const &conn);

  /**
   * Send @p message into @p conn
   *
   * \param message Protobuf format message(generated by protoc, ie. *.pb.h)
   */
  KANON_INLINE void Send(TcpConnectionPtr const &conn,
                         ::google::protobuf::Message const *message)
  {
    return Send(conn.get(), message);
  }

  void Send(TcpConnection *const conn,
            ::google::protobuf::Message const *message);

  KANON_INLINE void Send(TcpConnectionPtr const &conn,
                         ::google::protobuf::Message const &msg)
  {
    return Send(conn.get(), &msg);
  }

  KANON_INLINE void Send(TcpConnection *const conn,
                         ::google::protobuf::Message const &msg)
  {
    return Send(conn, &msg);
  }

  /*--------------------------------------------------*/
  /* Callback register                                */
  /*--------------------------------------------------*/

  void SetMessageCallback(MessageCallback cb)
  {
    message_callback_ = std::move(cb);
  }
  void SetErrorCallback(ErrorCallback cb) { error_callback_ = std::move(cb); }

 private:
  static KANON_INLINE uint32_t GetCheckSum(void const *buffer,
                                           size_t len) noexcept;
  static KANON_INLINE bool VerifyCheckSum(Buffer &buffer,
                                          SizeHeaderType size_header);
  static KANON_INLINE char const *ErrorToString(ErrorCode err) noexcept;

  /**
   * Identify specific message type
   * This should be unique identifier
   */
  StringView tag_;

  /** The maximum size of the message */
  size_t max_size_;

  /** Handle message of payload */
  MessageCallback message_callback_;

  /** Handle error occurred in parsing */
  ErrorCallback error_callback_;
};

bool ParseFromBuffer(::google::protobuf::Message *message, size_t payload_size,
                     Buffer *buffer);
bool ParsePartialFromBuffer(::google::protobuf::Message *message,
                            size_t payload_size, Buffer *buffer);

#define DEF_SPECIFIC_TAG_PROTOBUF_CODEC(codec_name_, tag_, msize_)             \
  class codec_name_ : public ::kanon::protobuf::ProtobufCodec2 {               \
   public:                                                                     \
    codec_name_()                                                              \
      : ::kanon::protobuf::ProtobufCodec2((tag_), (msize_))                    \
    {                                                                          \
    }                                                                          \
  }

} // namespace protobuf
} // namespace kanon

#endif