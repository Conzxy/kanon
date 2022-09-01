#ifndef KANON_NET_CHUNK_LIST_H
#define KANON_NET_CHUNK_LIST_H

#include "kanon/string/string_view.h"
#include "kanon/util/noncopyable.h"
#include "kanon/util/mem.h"
#include "kanon/algo/forward_list.h"
#include "kanon/net/sock_api.h"

namespace kanon {

/**
 * \brief variable-length buffer
 * Because output buffer need prepend length header, allocate 
 * fixed chunk is a waste.
 * There is no field of char[] or char*, use the padding as buffer
 * since node is not required to be continuous.
 * There are two advantages:
 * 1) Use char*, the locality is wrong than char[] or padding space and occupy space of pointer size
 * 2) Use char[], must use fixed-size
 */
class Chunk {
public:
  explicit Chunk(size_t sz=4096)
    : read_index_(0)
    , write_index_(0)
    , max_size_(sz)
  { 
  }

  ~Chunk() = default;

  void AdvanceRead(size_t len) noexcept {
    assert(read_index_ <= write_index_);
    read_index_ += len;
    assert(read_index_ <= write_index_);
  }

  /* len is signed integer
     allow back write index */
  void AdvanceWrite(int len) noexcept {
    assert(write_index_ <= max_size_);
    write_index_ += len;
    assert(write_index_ <= max_size_);
  }

  void AdvanceReadAll() noexcept {
    AdvanceRead(GetReadableSize());
  }

  void AdvanceWriteAll() noexcept {
    AdvanceWrite(GetWritableSize());
  }

  void Append(void const* data, size_t len) noexcept {
    assert(len <= GetWritableSize());
    ::memcpy(GetBuf()+write_index_, data, len);
    write_index_ += len;
  }

  StringView ToStringView() const noexcept {
    return StringView(GetBuf()+read_index_, write_index_-read_index_);
  }

  char* GetReadBegin() noexcept { return GetBuf() + read_index_; }
  char* GetWriteBegin() noexcept { return GetBuf() + write_index_; }
  char const* GetReadBegin() const noexcept { return GetBuf() + read_index_; }
  char const* GetWriteBegin() const noexcept { return GetBuf() + write_index_; }
  size_t GetReadableSize() const noexcept { return write_index_ - read_index_; }
  size_t GetWritableSize() const noexcept { return max_size_ - write_index_; }
  size_t GetMaxSize() const noexcept { return max_size_; }
  
  void Reset() noexcept {
    read_index_ = write_index_ = 0;
  }

private:
  // The buffer in the start position of the padding
  char* GetBuf() noexcept { return reinterpret_cast<char*>(this) + sizeof(Chunk); }
  char const* GetBuf() const noexcept { return reinterpret_cast<const char*>(this) + sizeof(Chunk); }

  size_t read_index_;
  size_t write_index_;
  size_t max_size_;

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
  static constexpr unsigned CHUNK_SIZE = 4096;

  using iterator = ListType::iterator;
  using const_iterator = ListType::const_iterator;

  using SizeType = ListType::size_type;

  // The ForwardList is an internel class
  ChunkList() = default;
  ~ChunkList() noexcept = default;
  
  // Explicit copy API to avoid implicit copy
 
  ChunkList Clone() {
    ChunkList ret;
    new (&ret.buffers_) ListType(buffers_);
    new (&ret.free_buffers_) ListType(free_buffers_);
    return ret;
  }

  void CopyAssign(ChunkList const &rhs) {
    buffers_ = rhs.buffers_;
    free_buffers_ = rhs.free_buffers_;
    *this = rhs; 
  }
  
  // Disable implicit copy
  // Dummy copy special function to move 
  // chunklist to callable
  // since std::function requires callable must be copyable
  // (type erasure need "virtual" copy constructor to support copy)
  ChunkList(ChunkList const&) { assert(false); }
  ChunkList& operator=(ChunkList const&) { assert(false); return *this; }
  ChunkList(ChunkList&& other) noexcept = default;
  ChunkList& operator=(ChunkList&& other) noexcept = default;

  void Append(void const* data, size_t len);
  void Append(StringView data) { Append(data.data(), data.size()); }

#define AppendInt_Macro(size) \
  void Append##size(uint##size##_t i) { \
    auto ni = sock::ToNetworkByteOrder##size(i); \
    Append(&ni, sizeof ni); \
  }

  void Append8(uint8_t i) {
    Append(&i, sizeof i);
  }

  AppendInt_Macro(16)
  AppendInt_Macro(32)
  AppendInt_Macro(64)

/**
 * Prepend a new chunk 
 */
#define PrependInt_Macro(size) \
  void Prepend##size(uint##size##_t i) noexcept { \
    buffers_.push_front(buffers_.create_node_size(sizeof(i), sizeof(i))); \
    assert(buffers_.front().GetMaxSize() == sizeof(i)); \
    auto ni = sock::ToNetworkByteOrder##size(i); \
    assert(sock::ToHostByteOrder##size(ni) == i); \
    buffers_.front().Append(&ni, sizeof ni); \
    assert(buffers_.front().GetReadableSize() == sizeof(i)); \
  }

  void Prepend8(uint8_t i) {
    buffers_.push_front(buffers_.create_node_size(sizeof(i), sizeof(i)));
    assert(buffers_.front().GetMaxSize() == sizeof(i));
    buffers_.front().Append(&i, sizeof i);
  }

  PrependInt_Macro(16)
  PrependInt_Macro(32)
  PrependInt_Macro(64)

#define GetReadBeginInt_Macro(size) \
  uint##size##_t GetReadBegin##size() noexcept { \
    assert(buffers_.front().GetReadableSize() >= sizeof(uint##size##_t)); \
    return sock::ToHostByteOrder##size(*reinterpret_cast<uint##size##_t*>(buffers_.front().GetReadBegin())); \
  }

  uint8_t GetReadBegin8() noexcept {
    assert(buffers_.front().GetReadableSize() >= sizeof(uint8_t));
    return *reinterpret_cast<uint8_t*>(buffers_.front().GetReadBegin());
  } 
  
  GetReadBeginInt_Macro(16)
  GetReadBeginInt_Macro(32)
  GetReadBeginInt_Macro(64)
  
#define ReadInt_Macro(size) \
  uint##size##_t Read##size() noexcept { \
    auto ret = GetReadBegin##size(); \
    AdvanceRead(sizeof(uint##size##_t)); \
    return ret; \
  }
  
  ReadInt_Macro(8)
  ReadInt_Macro(16)
  ReadInt_Macro(32)
  ReadInt_Macro(64)

  void AdvanceRead(size_t len);
  void AdvanceReadAll() { AdvanceRead(GetReadableSize()); }
  void Shrink(size_t chunk_size);
  void ReserveFreeChunk(size_t chunk_size);
  void ReserveWriteChunk(size_t chunk_size);

  SizeType GetChunkSize() const noexcept { return buffers_.size(); }
  SizeType GetFreeChunkSize() const noexcept { return free_buffers_.size(); }
  SizeType GetReadableSize() const noexcept;

  bool IsEmpty() const noexcept { return buffers_.empty(); }
  bool HasReadable() const noexcept { return !IsEmpty(); }
  

  Chunk* GetFirstChunk() noexcept { return &buffers_.front(); }
  Chunk const* GetFirstChunk() const noexcept { return &buffers_.front(); }
  Chunk* GetLastChunk() noexcept { return &buffers_.back(); }
  Chunk const* GetLastChunk() const noexcept { return &buffers_.back(); }

  iterator begin() noexcept {
    return buffers_.begin();
  }

  iterator end() noexcept {
    return buffers_.end();
  }

  const_iterator begin() const noexcept {
    return buffers_.begin();
  }

  const_iterator end() const noexcept {
    return buffers_.end();
  }

  const_iterator cbegin() const noexcept {
    return buffers_.begin();
  }

  const_iterator cend() const noexcept {
    return buffers_.end();
  }

  SizeType WriteFd(int fd, int& saved_errno) noexcept;
  SizeType WriteFd(int fd) noexcept { int saved_errno; return WriteFd(fd, saved_errno); }
  void swap(ChunkList& other) noexcept {
    buffers_.swap(other.buffers_);
    free_buffers_.swap(other.free_buffers_);
  }

  // void SetFreeMaxSize(size_t max_size) noexcept { free_max_size_ = max_size; }
  // size_t GetFreeMaxSize() const noexcept { return free_max_size_; }
  static SizeType GetSingleChunkSize() noexcept { return CHUNK_SIZE; }

private:

  bool PutToFreeChunk() noexcept;
  ListType::Iterator GetFreeChunk() noexcept;

  ListType buffers_;
  ListType free_buffers_;
};

inline void swap(ChunkList& lhs, ChunkList& rhs) noexcept {
  lhs.swap(rhs);
}

} // namespace kanon

#endif // KANON_NET_CHUNK_LIST_H
