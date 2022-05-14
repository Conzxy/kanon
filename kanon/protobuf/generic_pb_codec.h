#ifndef KANON_PROTOBUF_GENERIC_CODEC_H_
#define KANON_PROTOBUF_GENERIC_CODEC_H_

#include <string.h>

#include <functional>
#include <memory>

#include <unistd.h>
#include <zlib.h> // adler32

#include "kanon/log/logger.h"
#include "kanon/net/endian_api.h"
#include "kanon/string/string_view.h"
#include "kanon/util/noncopyable.h"
#include "kanon/net/callback.h"

#define PROTOBUF ::google::protobuf

namespace google {
namespace protobuf {

// fwd
class Message;

} // namespace protobuf
} // namespace google

namespace kanon { 

class Buffer;

namespace protobuf {

// FIXME Should use std::shared_ptr?
// using MessagePtr = std::unique_ptr<PROTOBUF::Message>;
using MessagePtr = PROTOBUF::Message*;

namespace internal {

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
 * \note 
 * 1. checksum compute the (size, tag, payload) content
 * 2. Only the payload is protobuf-format, others are just some trivial binary data
 *
 * \warning 
 * The class just an internal class, You should use ProtobufCodec<>
 */
class GenericPbCodec : noncopyable {
  enum ErrorCode {
    kNoError = 0,
    kParseError, /** protobuf parse */
    kInvalidLength, /** size header in raw format is invalid */
    kInvalidMessage, /** tag is unknown */
    kInvalidChecksum, /** checksum is don't match ==> raw data may be corrupted*/
  };

public:
  using MessageCallback = 
    std::function<void(TcpConnectionPtr const&, MessagePtr, TimeStamp)>;

  using ErrorCallback = 
    std::function<void(TcpConnectionPtr const&, ErrorCode)>;
  

  GenericPbCodec(
    PROTOBUF::Message const* prototype,
    std::string const& tag);

  ~GenericPbCodec();
  
  /**
   * Send @p message into @p conn
   * \param message Must be some concrete message type
   * \note message usually is a pointer in heap, so use pointer as non-optional parameter
   */
  void Send(TcpConnectionPtr const& conn, PROTOBUF::Message const* message);
  
  /**
   * Decode the raw message, which is protobuf-format binary data
   * \param buffer Contains the raw binary data
   */
  void OnMessage(TcpConnectionPtr const& conn, Buffer& buffer, TimeStamp receive_time);

  /**
   * Serialize @p message to @p buffer
   */
  uint32_t SerializeToBuffer(PROTOBUF::Message const& message, Buffer& buffer);

  /**
   * Deserialize @p buffer to @p message in @p length
   * \note This is just a wrapper
   */
  bool ParseFromBuffer(char const* buffer, int length, PROTOBUF::Message& message);

  /**
   * Check whether the @p message is valid, including the size, checksum, and tag.
   */
  ErrorCode Parse(char const* buffer, uint32_t size, PROTOBUF::Message& message);

  // Setter
  void SetMessageCallback(MessageCallback cb) noexcept { message_callback_ = std::move(cb); }
  void SetErrorCallback(ErrorCallback cb) noexcept { error_callback_ = std::move(cb); }

private: /** Helper */
  /** 
   * adler 
   * An Adler-32 checksum is almost as reliable as a CRC-32 but can be computed much faster.
   * \see ${USER_PATH}/include/zlib.h, in linux, ${USER_PATH} might be /usr
   */
  static inline uint32_t GetCheckSum(void const* buffer, int len) noexcept;
  static bool CheckCheckSum(void const* buffer, int len) noexcept;

  static char const* ErrorToString(ErrorCode err) noexcept;

  // For debugging
  void PrintRawMessage(kanon::Buffer& buffer);
private:
  static constexpr uint32_t kSizeLength = sizeof(uint32_t);
  static constexpr uint32_t kChecksumLength = sizeof(uint32_t);
  static constexpr uint64_t kMinMessageLength = kSizeLength + kChecksumLength;
  static constexpr uint64_t kMaxMessageLength = (1 << 26); // 64M

  /** Used for creating instance */
  PROTOBUF::Message const* prototype_;  
  
  /**
   * Identify specific message type
   * This should be unique identifier
   */
  const std::string tag_;
  
  /** Handle message of payload */
  MessageCallback message_callback_;

  /** Handle error occurred in parsing */
  ErrorCallback error_callback_;
};

uint32_t GenericPbCodec::GetCheckSum(void const* buffer, int len) noexcept
{
  /**
   * ZEXTERN uLong ZEXPORT adler32 OF((uLong adler, const Bytef *buf, uInt len));
   *
   * The first parameter used for updating, initial value is 0 usually.
   *
   * Update a running Adler-32 checksum with the bytes buf[0..len-1] and
   * return the updated checksum.  If buf is Z_NULL, this function returns the
   * required initial value for the checksum.
   *
   * Example:
   * uLong adler = adler32(0L, Z_NULL, 0);
   * while (read_buffer(buffer, length) != EOF) {
   *   adler = adler32(adler, buffer, length);
   * }
   * if (adler != original_adler) error();
   */
  return static_cast<uint32_t>(
    ::adler32(0, reinterpret_cast<Bytef const*>(buffer), len));
}

} // namespace internal

} } // namespace kanon::protobuf

#endif // KANON_PROTOBUF_GENERIC_CODEC_H_
