#ifndef KANON_NET_CHUNK_LIST_H
#define KANON_NET_CHUNK_LIST_H

#include "kanon/util/macro.h"
#include "kanon/string/string_view.h"
#include "kanon/util/mem.h"
#include "kanon/algo/forward_list.h"
#include "kanon/util/endian_api.h"

namespace kanon {

class ChunkList;

/**
 * \brief variable-length buffer
 * Because output buffer need prepend length header, allocate
 * fixed chunk is a waste.
 * There is no field of char[] or char*, use the padding as buffer
 * since node is not required to be continuous.
 * There are two advantages:
 * 1) Use char*, the locality is wrong than char[] or padding space and occupy
 * space of pointer size 2) Use char[], must use fixed-size
 */
class KANON_CORE_NO_API Chunk {
  friend class ChunkList;

 public:
  using size_type = unsigned int;

  explicit Chunk(size_type sz = 4096)
    : read_index_(0)
    , write_index_(0)
    , max_size_(sz)
  {
  }

  ~Chunk() = default;

  void SetToHeader() KANON_NOEXCEPT
  {
    read_index_ = write_index_ = sizeof(size_t);
  }

  void AdvanceRead(size_type len) KANON_NOEXCEPT
  {
    assert(read_index_ <= write_index_);
    read_index_ += len;
    assert(read_index_ <= write_index_);
  }

  void AdvanceWrite(size_type len) KANON_NOEXCEPT
  {
    assert(write_index_ <= max_size_);
    write_index_ += len;
    assert(write_index_ <= max_size_);
  }

  void AdvanceReadAll() KANON_NOEXCEPT { AdvanceRead(GetReadableSize()); }

  void AdvanceWriteAll() KANON_NOEXCEPT { AdvanceWrite(GetWritableSize()); }

  void Append(void const *data, size_type len) KANON_NOEXCEPT
  {

    assert(len <= GetWritableSize());
    ::memcpy(GetBuf() + write_index_, data, len);
    write_index_ += len;
  }

  StringView ToStringView() const KANON_NOEXCEPT
  {
    return StringView(GetBuf() + read_index_, write_index_ - read_index_);
  }

  char *GetReadBegin() KANON_NOEXCEPT { return GetBuf() + read_index_; }
  char *GetWriteBegin() KANON_NOEXCEPT { return GetBuf() + write_index_; }
  char const *GetReadBegin() const KANON_NOEXCEPT
  {
    return GetBuf() + read_index_;
  }
  char const *GetWriteBegin() const KANON_NOEXCEPT
  {
    return GetBuf() + write_index_;
  }
  size_type GetReadableSize() const KANON_NOEXCEPT
  {
    return write_index_ - read_index_;
  }
  size_type GetWritableSize() const KANON_NOEXCEPT
  {
    return max_size_ - write_index_;
  }
  size_type GetMaxSize() const KANON_NOEXCEPT { return max_size_; }

  void Reset() KANON_NOEXCEPT { read_index_ = write_index_ = 0; }

  void Prepend(void const *data, size_t len) KANON_NOEXCEPT
  {
    assert(read_index_ >= sizeof(size_t));
    assert(write_index_ >= sizeof(size_t));
    assert(len <= sizeof(size_t));

    read_index_ -= len;
    memcpy(GetReadBegin(), data, len);
  }

  bool IsHeaderInitState() const KANON_NOEXCEPT
  {
    return (sizeof(size_t) == read_index_) && (read_index_ == write_index_);
  }

 protected:
  // The buffer in the start position of the padding
  char *GetBuf() KANON_NOEXCEPT
  {
    return reinterpret_cast<char *>(this) + sizeof(Chunk);
  }
  char const *GetBuf() const KANON_NOEXCEPT
  {
    return reinterpret_cast<const char *>(this) + sizeof(Chunk);
  }

  size_type read_index_;
  size_type write_index_;
  size_type max_size_;

  // padding buffer
};

/**
 * \brief Represents a fixed-size block linked-list
 *
 * The node is a variable-size buffer chunk.
 * Use the chunked continuous, we can achieve zero-copy.
 * The buffer region in chunk is not occupy any space of a node,
 * need create intrusive nodes and append them.
 * Therefore, std::forward_list<> can't satisfy my requirements:
 * 1) There is no size() and xxx_back() methods to construct queue
 * 2) There is no APIs that can create instrusive nodes
 * On the contrary, in the basic of std::forward_list<>,
 * I implemete zstl::ForwardList<> to meet these requirements.
 *
 * \note
 *   Public class
 *   copyable and moveable
 */
class ChunkList final {
  using ListType = zstl::ForwardList<Chunk>;

 public:
/* I don't define CHUNK_SIZE and CHUNK_HEADER_SIZE to static constexpr
 * variable since use it to call create_node_size() will requires its
 * definition.
 *
 * such case is called ord-use, must take its address to read or write.
 * However, I only want it be a inline constant.
 *
 * So, I define them be macros is more convenient.
 * \see https://en.cppreference.com/w/cpp/language/definition#ODR-use
 */
#define CHUNK_SIZE 4096
  static_assert(CHUNK_SIZE >= sizeof(size_t),
                "The chunk size must be >= sizeof(size_t), ie. the maximum "
                "word size of your machine");

#define CHUNK_HEADER_SIZE (CHUNK_SIZE)
  // constexpr static unsigned CHUNK_SIZE = 4096;
  // constexpr static unsigned CHUNK_HEADER_SIZE = CHUNK_SIZE + sizeof(size_t);

  using iterator = ListType::iterator;
  using const_iterator = ListType::const_iterator;

  using SizeType = ListType::size_type;

  // The ForwardList is an internel class
  ChunkList() = default;
  KANON_CORE_API ~ChunkList() KANON_NOEXCEPT;

  // Explicit copy API to avoid implicit copy

  ChunkList Clone()
  {
    ChunkList ret;
    new (&ret.buffers_) ListType(buffers_);
    new (&ret.free_buffers_) ListType(free_buffers_);
    return ret;
  }

  void CopyAssign(ChunkList const &rhs)
  {
    buffers_ = rhs.buffers_;
    free_buffers_ = rhs.free_buffers_;
    *this = rhs;
  }

  // Disable implicit copy
  // Dummy copy special function to move
  // chunklist to callable
  // since std::function requires callable must be copyable
  // (type erasure need "virtual" copy constructor to support copy)
  ChunkList(ChunkList const &) { assert(false); }
  ChunkList &operator=(ChunkList const &)
  {
    assert(false);
    return *this;
  }
  ChunkList(ChunkList &&other) KANON_NOEXCEPT = default;
  ChunkList &operator=(ChunkList &&other) KANON_NOEXCEPT = default;

  KANON_CORE_API void AppendChunkList(ChunkList *rhs);
  KANON_CORE_API void Append(void const *data, size_t len);
  void Append(StringView data) { Append(data.data(), data.size()); }

#define KANON_CHUNK_LIST_APPEND_UINT(size)                                     \
  void Append##size(uint##size##_t i)                                          \
  {                                                                            \
    auto ni = sock::ToNetworkByteOrder##size(i);                               \
    Append(&ni, sizeof ni);                                                    \
  }

  void Append8(uint8_t i) { Append(&i, sizeof i); }

  KANON_CHUNK_LIST_APPEND_UINT(16)
  KANON_CHUNK_LIST_APPEND_UINT(32)
  KANON_CHUNK_LIST_APPEND_UINT(64)

/**
 * Prepend a new chunk
 */
#define KANON_CHUNK_LIST_PREPEND_UINT(size)                                    \
  void Prepend##size(uint##size##_t i) KANON_NOEXCEPT                          \
  {                                                                            \
    auto ni = sock::ToNetworkByteOrder##size(i);                               \
    assert(sock::ToHostByteOrder##size(ni) == i);                              \
    Prepend(&ni, sizeof(ni));                                                  \
  }

  void Prepend(void const *data, size_t len);

  KANON_CHUNK_LIST_PREPEND_UINT(8)
  KANON_CHUNK_LIST_PREPEND_UINT(16)
  KANON_CHUNK_LIST_PREPEND_UINT(32)
  KANON_CHUNK_LIST_PREPEND_UINT(64)

#define KANON_CHUNK_LIST_GET_READ_BEGIN_UINT(size)                             \
  uint##size##_t GetReadBegin##size() KANON_NOEXCEPT                           \
  {                                                                            \
    assert(buffers_.front().GetReadableSize() >= sizeof(uint##size##_t));      \
    return sock::ToHostByteOrder##size(                                        \
        *reinterpret_cast<uint##size##_t *>(buffers_.front().GetReadBegin())); \
  }

  uint8_t GetReadBegin8() KANON_NOEXCEPT
  {
    assert(buffers_.front().GetReadableSize() >= sizeof(uint8_t));
    return *reinterpret_cast<uint8_t *>(buffers_.front().GetReadBegin());
  }

  KANON_CHUNK_LIST_GET_READ_BEGIN_UINT(16)
  KANON_CHUNK_LIST_GET_READ_BEGIN_UINT(32)
  KANON_CHUNK_LIST_GET_READ_BEGIN_UINT(64)

#define KANON_CHUNK_LIST_READ_UINT_(size)                                      \
  uint##size##_t Read##size() KANON_NOEXCEPT                                   \
  {                                                                            \
    auto ret = GetReadBegin##size();                                           \
    AdvanceRead(sizeof(uint##size##_t));                                       \
    return ret;                                                                \
  }

  KANON_CHUNK_LIST_READ_UINT_(8)
  KANON_CHUNK_LIST_READ_UINT_(16)
  KANON_CHUNK_LIST_READ_UINT_(32)
  KANON_CHUNK_LIST_READ_UINT_(64)

  KANON_CORE_API void AdvanceRead(size_t len);
  void AdvanceReadAll() { AdvanceRead(GetReadableSize()); }
  KANON_CORE_API void Shrink(size_t chunk_size);
  void ShrinkChunk(size_t chunk_size) { Shrink(chunk_size); }

  KANON_CORE_API void ReserveFreeChunk(size_t chunk_size);
  KANON_INLINE void ReserveChunk(size_t chunk_size)
  {
    while (chunk_size--) {
      buffers_.push_back(buffers_.create_node_size(CHUNK_SIZE));
    }
  }

  KANON_CORE_API void ReserveWriteSpace(size_t size);

  SizeType GetChunkSize() const KANON_NOEXCEPT { return buffers_.size(); }
  SizeType GetFreeChunkSize() const KANON_NOEXCEPT
  {
    return free_buffers_.size();
  }
  KANON_CORE_API SizeType GetReadableSize() const KANON_NOEXCEPT;

  bool IsEmpty() const KANON_NOEXCEPT { return buffers_.empty(); }
  bool HasReadable() const KANON_NOEXCEPT { return !IsEmpty(); }

  Chunk *GetFirstChunk() KANON_NOEXCEPT { return &buffers_.front(); }
  Chunk const *GetFirstChunk() const KANON_NOEXCEPT
  {
    return &buffers_.front();
  }
  Chunk *GetLastChunk() KANON_NOEXCEPT { return &buffers_.back(); }
  Chunk const *GetLastChunk() const KANON_NOEXCEPT { return &buffers_.back(); }

  iterator begin() KANON_NOEXCEPT { return buffers_.begin(); }

  iterator end() KANON_NOEXCEPT { return buffers_.end(); }

  const_iterator begin() const KANON_NOEXCEPT { return buffers_.begin(); }

  const_iterator end() const KANON_NOEXCEPT { return buffers_.end(); }

  const_iterator cbegin() const KANON_NOEXCEPT { return buffers_.begin(); }

  const_iterator cend() const KANON_NOEXCEPT { return buffers_.end(); }

  void swap(ChunkList &other) KANON_NOEXCEPT
  {
    buffers_.swap(other.buffers_);
    free_buffers_.swap(other.free_buffers_);
  }

  // void SetFreeMaxSize(size_t max_size) KANON_NOEXCEPT { free_max_size_ =
  // max_size; } size_t GetFreeMaxSize() const KANON_NOEXCEPT { return
  // free_max_size_; }
  static SizeType GetSingleChunkSize() KANON_NOEXCEPT { return CHUNK_SIZE; }

  void DebugPrint();

 private:
  bool PutToFreeChunk() KANON_NOEXCEPT;
  ListType::Iterator GetFreeChunk() KANON_NOEXCEPT;

  void PushHeader()
  {
    assert(buffers_.empty());
    buffers_.push_back(
        buffers_.create_node_size(CHUNK_HEADER_SIZE, CHUNK_HEADER_SIZE));
    buffers_.begin()->SetToHeader();
  }

  Chunk *AddChunk();

  /**
   * buffers_ contains a chunk header and fixed payload chunks.
   * +--------+     +-------------------+
   * | header | ->  | payload(4096here) |
   * +--------+     +-------------------+
   */
  ListType buffers_;

  /**
   * free_buffers contains the allocated chunks.
   * Default, the consumed chunks in buffers_ will be pushed
   * into this to reuse.
   *
   * User can call Shrink()/ShrinkChunk() to reclaim the
   * free chunks.
   */
  ListType free_buffers_;
};

KANON_INLINE void swap(ChunkList &lhs, ChunkList &rhs) KANON_NOEXCEPT
{
  lhs.swap(rhs);
}

} // namespace kanon

#endif // KANON_NET_CHUNK_LIST_H
