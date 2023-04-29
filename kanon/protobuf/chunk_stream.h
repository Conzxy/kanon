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
  // clang-format off
  /**
   * Get the avaliable chunk of the stream.
   * The chunk contains the writable bytes in \p size, 
   * the \p data point to the first bytes of the chunk.
   *
   * The protobuf docs:
   * -----------------------------------------------------------------------------
   * Obtains a buffer into which data can be written.
   * Any data written into this buffer will eventually (maybe instantly, maybe later on) be written to the output.
   * 
   * Preconditions:
   * - "size" and "data" are not NULL.
   * Postconditions:
   * - If the returned value is false, an error occurred. All errors are permanent.
   * - Otherwise, "size" points to the actual number of bytes in the buffer and "data" points to the buffer.
   * - Ownership of this buffer remains with the stream, and the buffer remains valid only until some other method of the stream is called or the stream is destroyed.
   * - Any data which the caller stores in this buffer will eventually be written to the output (unless BackUp() is called).
   * - It is legal for the returned buffer to have zero size, as long as repeatedly calling Next() eventually yields a buffer with non-zero size.
   * ------------------------------------------------------------------------------\
   *
   * \see https://protobuf.dev/reference/cpp/api-docs/google.protobuf.io.zero_copy_stream/#ZeroCopyOutputStream.Next.details
   */
  // clang-format on
  bool Next(void **data, int *size) override;

  // clang-format off
  /**
   * To `ChunkList`, protobuf don't know the implementation detail about it.
   * So, I have to suppose protobuf will fill the chunk obtained by calling
   * Next(). If protobuf don't fill it, it will call BackUp() to rewind the
   * write index to correct index, making the state of `ChunkList` is consistent.
   *
   * The protobuf docs:
   * ----------------------------------------------------------------------
   * Backs up a number of bytes, so that the end of the last buffer returned by Next() is not actually written.
   * This is needed when you finish writing all the data you want to write, but the last buffer was bigger than you needed. You don't want to write a bunch of garbage after the end of your data, so you use BackUp() to back up.
   * 
   * Preconditions:
   * 
   *  - The last method called must have been Next().
   *  - count must be less than or equal to the size of the last buffer returned by Next().
   *  - The caller must not have written anything to the last "count" bytes of that buffer.
   * 
   * Postconditions:
   * 
   *  - The last "count" bytes of the last buffer returned by Next() will be ignored
   * ----------------------------------------------------------------------
   *
   * \see
   * https://protobuf.dev/reference/cpp/api-docs/google.protobuf.io.zero_copy_stream/#ZeroCopyInputStream.BackUp.details
   */
  // clang-format on
  void BackUp(int count) override;

  /**
   * Get the readable size of the chunk list
   */
  int64_t ByteCount() const override;

  // clang-format off
  /*
   * \note SerializeToZeroCopyStream() don't call WriteAliasedRaw()
   *
   * The protobuf docs:
   * -----------------------------------------------------------------------
   * Write a given chunk of data to the output.
   * 
   *   Some output streams may implement this in a way that avoids copying. Check AllowsAliasing() before calling WriteAliasedRaw(). It will GOOGLE_CHECK fail if WriteAliasedRaw() is called on a stream that does not allow aliasing.
   * 
   *   NOTE: It is caller's responsibility to ensure that the chunk of memory remains live until all of the data has been consumed from the stream.
   * -----------------------------------------------------------------------
   * \see https://protobuf.dev/reference/cpp/api-docs/google.protobuf.io.zero_copy_stream/#ZeroCopyOutputStream.WriteAliasedRaw.details
   */
  // clang-format on
  bool WriteAliasedRaw(void const *data, int size) override;

  bool AllowsAliasing() const override { return true; }

  ChunkList chunk_list;
};

} // namespace protobuf
} // namespace kanon
#endif
