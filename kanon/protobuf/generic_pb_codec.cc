#include "generic_pb_codec.h"

#include <google/protobuf/message.h>

#include "chunk_stream.h"
#include "kanon/log/logger.h"
#include "kanon/net/connection/tcp_connection.h"
#include "kanon/net/endian_api.h"

#include <xxHash/xxhash.h>

using namespace kanon;
using namespace kanon::protobuf::internal;
using namespace google::protobuf;

GenericPbCodec::GenericPbCodec(PROTOBUF::Message const *prototype,
                               std::string const &tag)
  : prototype_(prototype)
  , tag_(tag)
{
  // Default error callback
  // Log error and Disconnect peer
  SetErrorCallback([](TcpConnectionPtr const &conn, ErrorCode error_code) {
    LOG_ERROR << "Error occurred in error callback of GenericPbCodec: "
              << ErrorToString(error_code);
    if (conn && conn->IsConnected()) {
      conn->ShutdownWrite();
    }
  });
}

// implicit-defined noexcept
GenericPbCodec::~GenericPbCodec() = default;

void GenericPbCodec::Send(TcpConnectionPtr const &conn,
                          PROTOBUF::Message const *message)
{
#if 0
  ChunkList buffer;

  buffer.Append(tag_);

  // payload length
  uint32_t check_sum;
  const auto bytes = SerializeToBuffer(*message, buffer, check_sum);

  // Don't count the size header
  buffer.Prepend32(tag_.size() + bytes + kChecksumLength);
  buffer.Append32(check_sum);

#else
  ChunkStream stream;
  auto &buffer = stream.chunk_list();

  buffer.Append(tag_);
  assert(buffer.GetReadableSize() == tag_.size());
  /* The API will call ByteSizeLong(), i.e.
     force cache it, we can reuse it to avoid computation */
  message->SerializePartialToZeroCopyStream(&stream);
  uint32_t bytes = message->GetCachedSize();
  LOG_DEBUG << "bytes = " << bytes;
  assert(buffer.GetReadableSize() == bytes + tag_.size());

  auto state = XXH32_createState();
  auto ok = XXH32_reset(state, 0) != XXH_ERROR;
  KANON_ASSERT(ok, "XXH32_reset() error");

  auto chunk_count = buffer.GetChunkSize() - 1;
  auto beg = buffer.begin();
  while (chunk_count--) {
    beg->AdvanceWriteAll();
    ok = XXH32_update(state, beg->GetReadBegin(), beg->GetReadableSize()) !=
         XXH_ERROR;
    KANON_ASSERT(ok, "XXH32_update() error");
    ++beg;
  }
  assert(beg.next() == buffer.end());
  ok = XXH32_update(state, beg->GetReadBegin(), beg->GetReadableSize()) !=
       XXH_ERROR;
  KANON_ASSERT(ok, "XXH32_update() error");
  uint32_t check_sum = XXH32_digest(state);
  XXH32_freeState(state);
  LOG_DEBUG << "CheckSum = " << check_sum;

  buffer.Prepend32(tag_.size() + bytes + kChecksumLength);
  buffer.Append32(check_sum);

  LOG_DEBUG << "buffer readable size = " << buffer.GetReadableSize();
#endif
  assert(buffer.GetReadableSize() ==
         tag_.size() + kSizeLength + bytes + kChecksumLength);
  conn->Send(buffer);
}

void GenericPbCodec::OnMessage(TcpConnectionPtr const &conn, Buffer &buffer,
                               TimeStamp receive_time)
{
  /**
   * 1. Check the GetReadableSize() of buffer. If the length is greater than the
   * kMinMessageLength, extract the size header 2.1. Check whether the size
   * header is valid, if not, call error_callback_ 2.2. Check whether the
   * GetReadableSize() of buffer is greater than the size header+kSizeLength, if
   * not, just return and wait
   * 3. Parse the message and get return value which represents error code
   * 3.1 If error code is not kNoError, call error_callback_
   * 3.2 Otherwise, call message_callback_
   */

  // Use while here since maybe not just one message in buffer
  while (buffer.GetReadableSize() >= kMinMessageLength) {
    const uint32_t size_header = buffer.GetReadBegin32();
    LOG_DEBUG_KANON << "size_header = " << size_header;
    if (size_header < kChecksumLength || size_header >= kMaxMessageLength) {
      error_callback_(conn, kInvalidLength);
      break;
    } else if (buffer.GetReadableSize() >= kSizeLength + size_header) {
      // Coming a complete message
      // Message::New() is a virtual copy constructor(see prototype pattern)
      //
      // Don't call make_unique<>(), we just use the RAII property of
      // std::unique_ptr<>
      auto message = std::unique_ptr<PROTOBUF::Message>(prototype_->New());

      const auto error_code =
          Parse(buffer.GetReadBegin() + kSizeLength, size_header, *message);
      PrintRawMessage(buffer);

      if (error_code == kNoError) {
        message_callback_(conn, GetPointer(message), receive_time);
        buffer.AdvanceRead(kSizeLength + size_header);
      } else {
        error_callback_(conn, error_code);
        break;
      }
    } else {
      break;
    }
  }
}

auto GenericPbCodec::Parse(char const *buffer, uint32_t size,
                           PROTOBUF::Message &message) -> ErrorCode
{
  /**
   * 1. Check the checksum
   * 2. Check the tag
   * 3. Parse and check if it is successful
   * 4. return the proper error code
   */
  auto ret = kNoError;

  if (CheckCheckSum(buffer, size - kChecksumLength)) {
    if (0 == ::memcmp(buffer, tag_.data(), tag_.size())) {
      if (!ParseFromBuffer(buffer + tag_.size(),
                           size - kChecksumLength - tag_.size(), message))
      {
        ret = kParseError;
      } else {
        LOG_DEBUG_KANON << "[parse message] = " << message.DebugString();
      }
    } else {
      ret = kInvalidMessage;
    }

  } else {
    ret = kInvalidChecksum;
  }

  return ret;
}

bool GenericPbCodec::ParseFromBuffer(char const *buffer, int length,
                                     PROTOBUF::Message &message)
{
  return message.ParseFromArray(buffer, length);
}

uint32_t GenericPbCodec::SerializeToBuffer(PROTOBUF::Message const &message,
                                           Buffer &buffer)
{
  // ByteSize() return the bytes that has written into the message
  // GetCachedSize() return the ByteSize() last called
  const auto has_written_bytes = message.ByteSizeLong();
  buffer.ReserveWriteSpace(has_written_bytes);

  auto *begin = reinterpret_cast<uint8_t *>(buffer.GetWriteBegin());
  auto *last = message.SerializeWithCachedSizesToArray(begin);

  LOG_DEBUG_KANON << "[payload] = " << message.DebugString();

  if ((size_t)(last - begin) == has_written_bytes) {
    buffer.AdvanceWrite(has_written_bytes);
  } else {
    LOG_FATAL << "The return value of SerializeWithCachedSizesToArray() is not "
                 "same with "
                 "tht GetCachedSize(), since the internal error of google or "
                 "you problem?";
  }

  return has_written_bytes;
}

uint32_t GenericPbCodec::SerializeToBuffer(Message const &message,
                                           ChunkList &buffer,
                                           uint32_t &chunk_sum)
{
  const auto has_written_bytes = message.ByteSizeLong();

  /* FIXME Not efficient */
  auto raw_buffer = (google::protobuf::uint8 *)malloc(has_written_bytes);
  message.SerializeWithCachedSizesToArray(raw_buffer);
  chunk_sum = GetCheckSum(raw_buffer, has_written_bytes);
  buffer.Append(raw_buffer, has_written_bytes);
  free(raw_buffer);
  return has_written_bytes;
}

bool GenericPbCodec::CheckCheckSum(void const *buffer, int len) noexcept
{
  auto new_checksum = GetCheckSum(buffer, len);
  uint32_t old_checksum = 0;
  ::memcpy(&old_checksum, reinterpret_cast<char const *>(buffer) + len,
           kChecksumLength);
  old_checksum = sock::ToHostByteOrder32(old_checksum);

  LOG_DEBUG_KANON << "new_checksum = " << new_checksum;
  LOG_DEBUG_KANON << "old_checksum = " << old_checksum;
  return new_checksum == old_checksum;
}

char const *GenericPbCodec::ErrorToString(ErrorCode err) noexcept
{
  switch (err) {
  case kInvalidLength:
    return "Invalid Length";
  case kInvalidMessage:
    return "Invalid Message";
  case kInvalidChecksum:
    return "Invalid Checksum";
  case kParseError:
    return "Parse Error";
  case kNoError:
    return "No Error";
  default:
    return "Unknown Error";
  }
}

void GenericPbCodec::PrintRawMessage(Buffer &buffer)
{
  auto view = buffer.ToStringView();
  uint32_t checksum = 0;
  LOG_DEBUG_KANON << "[Size Header] = " << buffer.GetReadBegin32();
  LOG_DEBUG_KANON << "[tag] = " << view.substr(kSizeLength, tag_.size());

  auto checksum_view =
      view.substr(buffer.GetReadableSize() - kChecksumLength, kChecksumLength);
  ::memcpy(&checksum, checksum_view.data(), kChecksumLength);
  LOG_DEBUG_KANON << "[checksum] = " << sock::ToHostByteOrder32(checksum);
}

inline uint32_t GenericPbCodec::GetCheckSum(void const *buffer,
                                            int len) noexcept
{
  return XXH32(buffer, len, 0);
}