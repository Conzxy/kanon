#include "kanon/protobuf/protobuf_codec2.h"

#include <google/protobuf/message.h>

#include "chunk_stream.h"
#include "kanon/protobuf/logger.h"
#include "kanon/buffer/chunk_list.h"
#include "kanon/net/connection/tcp_connection.h"
#include "kanon/net/endian_api.h"

#include "kanon/protobuf/kvarint/kvarint.h"

#include <third-party/xxHash/xxhash.h>

using namespace kanon;
using namespace ::google::protobuf;
using namespace kanon::protobuf;

ProtobufCodec2::ProtobufCodec2(StringView tag, size_t max_size)
  : tag_(std::move(tag))
  , max_size_(max_size)
{
  // Default error callback
  // Log error and Disconnect peer
  SetErrorCallback([](TcpConnectionPtr const &conn, ErrorCode error_code) {
    LOG_ERROR_KANON_PROTOBUF
        << "Error occurred in error callback of ProtobufCodec2: "
        << ErrorToString(error_code);

    LOG_WARN_KANON_PROTOBUF
        << "If you want to do something response to peer(eg. send some message "
           "to your peer), "
           "you should register specific error callback by SetErrorCallback(), "
           "I can't do it since I don't know the concrete message type";
    if (conn && conn->IsConnected()) {
      conn->ShutdownWrite();
    }
  });
}

// implicit-defined noexcept
ProtobufCodec2::~ProtobufCodec2() noexcept {}

void ProtobufCodec2::SetUpConnection(TcpConnectionPtr const &conn)
{
  conn->SetMessageCallback([this](TcpConnectionPtr const &conn, Buffer &buffer,
                                  TimeStamp receive_time) {
    // Discard too large buffer early to avoid making memory overflow
    if (buffer.GetReadableSize() >= max_size_) {
      LOG_WARN_KANON_PROTOBUF << "A single message too large, just discard";
      buffer.AdvanceAll();
      buffer.Shrink();
      error_callback_(conn, E_INVALID_SIZE_HEADER);
      return;
    }

    uint32_t size_header = 0;
    size_t size_header_len = 0;
    // Use while here since maybe not just one message in buffer
    while (true) {
      const auto decode_ret =
          kvarint_decode32(buffer.GetReadBegin(), buffer.GetReadableSize(),
                           &size_header_len, &size_header);
      switch (decode_ret) {
        case KVARINT_OK:
          LOG_DEBUG_KANON_PROTOBUF
              << "This is a valid message encoded by varint";
          break;
        case KVARINT_DECODE_BUF_INVALID:
          error_callback_(conn, E_INVALID_MESSAGE);
          return;
        case KVARINT_DECODE_BUF_SHORT:
#ifndef NDEBUG
          if (buffer.GetReadableSize() > 0)
            LOG_DEBUG_KANON_PROTOBUF
                << "The size header is short, wait complete...";
#endif
          return;
      }

      LOG_DEBUG << "Size header = " << size_header;
      LOG_DEBUG << "Current readable size = " << buffer.GetReadableSize();

      // BUG FIX:
      // Invalid message length is untrusted.
      if (size_header < tag_.size() + CHECKSUM_LENGTH ||
          size_header >= max_size_ - size_header_len)
      {
        error_callback_(conn, E_INVALID_SIZE_HEADER);
        break;
      }

      KANON_ASSERT(
          buffer.GetReadableSize() > size_header_len,
          "Buffer's readable size must be >= the length of size_header");
      // Waiting complete message
      if (buffer.GetReadableSize() - size_header_len < size_header) break;

      buffer.AdvanceRead(size_header_len);

      // BUG FIX:
      // If peer send invalid message whose length over size_header and
      // MIN_SIZE, then can reach this. In this case, size_header is a
      // untrusted field. Such message should discard.

      if (!VerifyCheckSum(buffer, size_header)) {
        error_callback_(conn, E_INVALID_CHECKSUM);
        break;
      }

      if (::memcmp(buffer.GetReadBegin(), tag_.data(), tag_.size()) != 0) {
        error_callback_(conn, E_INVALID_MESSAGE);
        break;
      }

      buffer.AdvanceRead(tag_.size());

      message_callback_(conn, buffer,
                        size_header - tag_.size() - CHECKSUM_LENGTH,
                        receive_time);
      buffer.AdvanceRead32(); // checksum
    }
  });
}

void ProtobufCodec2::Send(TcpConnectionPtr const &conn,
                          ::google::protobuf::Message const *message)
{
#if 0
  ChunkList buffer;

  buffer.Append(tag_);

  // payload length
  uint32_t check_sum;
  const auto bytes = SerializeToBuffer(*message, buffer, check_sum);

  // Don't count the size header
  buffer.Prepend32(tag_.size() + bytes + CHECKSUM_LENGTH);
  buffer.Append32(check_sum);
#else
  ChunkStream stream;
  auto &buffer = stream.chunk_list;

  buffer.Append(tag_);
  assert(buffer.GetReadableSize() == tag_.size());
  /* The API will call ByteSizeLong(), i.e.
     force cache it, we can reuse it to avoid computation */
  message->SerializeToZeroCopyStream(&stream);
  uint32_t bytes = message->GetCachedSize();
  LOG_DEBUG_KANON_PROTOBUF << "The serialized payload bytes = " << bytes;
  assert(buffer.GetReadableSize() == bytes + tag_.size());

  auto state = XXH32_createState();
  auto ok = XXH32_reset(state, 0) != XXH_ERROR;
  KANON_UNUSED(ok);
  KANON_ASSERT(ok, "XXH32_reset() error");

  auto chunk_count = buffer.GetChunkSize() - 1;
  auto beg = buffer.begin();
  while (chunk_count--) {
    assert(beg->GetWritableSize() == 0);
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

  LOG_DEBUG_KANON_PROTOBUF << "CheckSum = " << check_sum;
  buffer.Append32(check_sum);

  kvarint_buf32_t kvarint_buf;
  kvarint_encode32(buffer.GetReadableSize(), &kvarint_buf);
  buffer.Prepend(kvarint_buf.buf, kvarint_buf.len);

  LOG_DEBUG_KANON_PROTOBUF << "buffer readable size = "
                           << buffer.GetReadableSize();
#endif
  assert(buffer.GetReadableSize() ==
         tag_.size() + kvarint_buf.len + bytes + CHECKSUM_LENGTH);
  conn->Send(buffer);
}

char const *ProtobufCodec2::ErrorToString(ErrorCode err) noexcept
{
  switch (err) {
    case E_NOERROR:
      return "INFO: OK";
    case E_INVALID_MESSAGE:
      return "FATAL: Invalid message";
    case E_INVALID_CHECKSUM:
      return "FATAL: Invalid CheckSum";
    case E_INVALID_SIZE_HEADER:
      return "FATAL: Invalid size header";
    case E_NO_COMPLETE_MESSAGE:
      return "INFO: This is not a error";
  }

  return "FATAL: Unknown error";
}

KANON_INLINE uint32_t ProtobufCodec2::GetCheckSum(void const *buffer,
                                                  size_t len) noexcept
{
  return XXH32(buffer, len, 0);
}

bool ProtobufCodec2::VerifyCheckSum(Buffer &buffer, SizeHeaderType size_header)
{
  LOG_DEBUG << "calculated range: " << size_header - CHECKSUM_LENGTH;

  const auto calculated_check_sum =
      XXH32(buffer.GetReadBegin(), size_header - CHECKSUM_LENGTH, 0);
  CheckSumType prepared_checksum = 0;
  ::memcpy(&prepared_checksum,
           buffer.GetReadBegin() + size_header - CHECKSUM_LENGTH,
           CHECKSUM_LENGTH);
  prepared_checksum = sock::ToHostByteOrder32(prepared_checksum);

  LOG_DEBUG << "calculated check sum = " << calculated_check_sum;
  LOG_DEBUG << "prepared check sum = " << prepared_checksum;

  return calculated_check_sum == prepared_checksum;
}

/* ------------------------------------------------------- */
bool kanon::protobuf::ParseFromBuffer(::google::protobuf::Message *message,
                                      size_t payload_size, Buffer *buffer)
{
  assert(payload_size <= buffer->GetReadableSize());
  auto ret = message->ParseFromArray(buffer->GetReadBegin(), payload_size);
  if (ret) buffer->AdvanceRead(payload_size);
  return ret;
}

bool kanon::protobuf::ParsePartialFromBuffer(
    ::google::protobuf::Message *message, size_t payload_size, Buffer *buffer)
{
  assert(payload_size <= buffer->GetReadableSize());
  auto ret =
      message->ParsePartialFromArray(buffer->GetReadBegin(), payload_size);
  if (ret) buffer->AdvanceRead(payload_size);
  return ret;
}
