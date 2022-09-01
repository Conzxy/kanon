#ifndef KANON_RPC_CHUNK_STREAM_H_
#define KANON_RPC_CHUNK_STREAM_H_

#include <google/protobuf/io/zero_copy_stream.h>

#include "kanon/buffer/chunk_list.h"

namespace kanon {
namespace protobuf {

/**
 * To optimize the performance of protobuf message serialization
 * and make it to be compatible with the output buffer of kanon
 * 
 * Implement the specific ZeroCopyOutputStream of the kanon
 * is necessary.
 * \see ZeroCopyOutputStream
 */
class ChunkStream : public ::google::protobuf::io::ZeroCopyOutputStream {
 public:
  /**
   * Get the avaliable chunk of the stream
   */
  bool Next(void **data, int *size) override;

  /**
   * The \p count bytes in the last chunk don't write into 
   * the stream
   * i.e. advance write index to correct value
   */
  void BackUp(int count) override;

  /**
   * Get the readable size of the chunk list
   */
  int64_t ByteCount() const override;

  /*
   * SerializeToZeroCopyStream() don't call WriteAliasedRaw()
   */
  bool WriteAliasedRaw(void const *data, int size) override;

  bool AllowsAliasing() const override  {
    return true;
  }

  ChunkList &chunk_list() noexcept
  {
    return chunk_list_;
  }
  ChunkList const &chunk_list() const noexcept
  {
    return chunk_list_;
  }

 private:
  ChunkList chunk_list_;
};

} // namespace protobuf
} // namespace kanon
#endif