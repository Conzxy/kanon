#include "chunk_list.h"

#include <limits.h>
#include <inttypes.h>

#include "kanon/util/macro.h"

// #define CHUNK_LIST_DEBUG

#include <stdio.h>

#ifdef CHUNK_LIST_DEBUG

#  define DLOG(str, ...) ::printf(str, __VA_ARGS__)
#else
#  define DLOG(str, ...)
#endif

#define ASSERT_HEADER                                                          \
  assert(CHUNK_HEADER_SIZE == buffers_.begin()->GetMaxSize());                 \
  assert(!buffers_.empty());                                                   \
  assert(buffers_.begin()->IsHeaderInitState())

namespace kanon {

ChunkList::~ChunkList() KANON_NOEXCEPT
{
  // if (!free_buffers_.empty()) {
  //   auto first_size = free_buffers_.front().GetMaxSize();
  //   // non-trivial chunk, process specially
  //   if (first_size != CHUNK_SIZE) buffers_.pop_front_size(first_size);
  // }
  buffers_.clear_size(CHUNK_SIZE);
  free_buffers_.clear_size(CHUNK_SIZE);
}

auto ChunkList::Prepend(void const *data, size_t len) -> void
{
  if (buffers_.empty()) {
    auto chunk = AddChunk();
    KANON_UNUSED(chunk);
  }

  buffers_.begin()->Prepend(data, len);
}

void ChunkList::Append(void const *data, size_t len)
{
  auto buf = reinterpret_cast<char const *>(data);

  /* Allow user append data even though the size header
   * has prepended. */

  // If buffers_ is not empty, try fill the the last chunk
  if (!buffers_.empty()) {
    DLOG("%s", "===== append to last chunk =====\n");
    auto last_block = buffers_.before_end();
    auto last_block_wsize = last_block->GetWritableSize();

    DLOG("Last chunk writable size = %zu\n", (size_t)last_block_wsize);
    /* Avoid a memcpy(buf, src, 0) */
    if (last_block_wsize > 0) {
      if (last_block_wsize >= len) {
        last_block->Append(buf, (Chunk::size_type)len);
        assert(last_block->GetWritableSize() == last_block_wsize - len);
        DLOG("len = %zu < writable size of last chunk\n", len);
        return;
      } else {
        last_block->Append(buf, last_block_wsize);
        len -= last_block_wsize;
        buf += last_block_wsize;
      }
    }
  }

  DLOG("%s", "===== Append to the back of buffers_ =====\n");
  while (len > 0) {
    auto free_buffer = AddChunk();
    const auto wsize = free_buffer->GetWritableSize();
    if (len >= wsize) {
      free_buffer->Append(buf, wsize);
      len -= wsize;
      buf += wsize;
    } else {
      free_buffer->Append(buf, (Chunk::size_type)len);
      return;
    }
  }
  KANON_ASSERT(GetLastChunk()->GetWritableSize() != CHUNK_SIZE,
               "The last chunk must not be empty");
}

void ChunkList::AdvanceRead(size_t len)
{
  ListType::Iterator first_block;

  /* To distinguish the header process and trivial chunk process,
   * must define variables.
   *
   * The condition is (has_process_header) ? buffers_.size() > 1 :
buffers_.size() > 0.
   * equivalent to (buffers_.size() > (has_process_header ? 1 : 0)).
   * define has_process_header to a integer is more actual. */

  // size_t has_process_header = 0;
  while (buffers_.size() > 0) {
    first_block = buffers_.begin();
    auto const rsize = first_block->GetReadableSize();
    DLOG("readable size = %zu\n", (size_t)rsize);
    // To the size() == 1, we also put it to free_buffers_
    // then we can append content in CHUNK_SIZE at most
    // since there is only one avaliable chunk to use
    // reuse also can avoid to call ::malloc()
    if (len >= first_block->GetReadableSize()) {
      if (!PutToFreeChunk()) {
      }

      len -= rsize;
    } else {
      first_block->AdvanceRead((Chunk::size_type)len);
      return;
    }
  }
}

auto ChunkList::GetReadableSize() const KANON_NOEXCEPT->SizeType
{
  if (IsEmpty()) {
    return 0;
  } else {
    auto first_chunk = buffers_.begin();
    if (buffers_.size() == 1) {
      return first_chunk->GetReadableSize();
    } else if (buffers_.size() == 2) {
      return first_chunk->GetReadableSize() +
             first_chunk.next()->GetReadableSize();
    }

    auto last_chunk = buffers_.before_end();
    // return first_chunk->GetMaxSize() != 4096
    //            ? first_chunk->GetReadableSize() +
    //                  first_chunk.next()->GetReadableSize() +
    //                  (GetChunkSize() - 3) * CHUNK_SIZE +
    //                  last_chunk->GetReadableSize()
    //            : first_chunk->GetReadableSize() +
    //                  (GetChunkSize() - 2) * CHUNK_SIZE +
    //                  last_chunk->GetReadableSize();
    return first_chunk->GetReadableSize() +
           first_chunk.next()->GetReadableSize() +
           (GetChunkSize() - 3) * CHUNK_SIZE + last_chunk->GetReadableSize();
  }
}

bool ChunkList::PutToFreeChunk() KANON_NOEXCEPT
{
  if (buffers_.front().GetMaxSize() != CHUNK_SIZE) {
    return false;
  }

  assert(!buffers_.empty());

  DLOG("%s\n", "Put a new free chunk");
  auto free_buf = buffers_.extract_front();
  assert(free_buf->GetMaxSize() == CHUNK_SIZE);
  free_buf->Reset();
  free_buffers_.push_front(free_buf);
  return true;
}

void ChunkList::Shrink(size_t chunk_size)
{
  if (chunk_size >= free_buffers_.size()) {
    return;
  }

  /* Consider the empty chunk header as a free chunk */
  // if (!buffers_.empty()) {
  //   auto header = buffers_.begin();
  //   if (header->IsHeaderInitState()) {
  //     buffers_.drop_node_size(buffers_.extract_front_node(),
  //     CHUNK_HEADER_SIZE);
  //     --chunk_size;
  //   }
  // }

  auto end = free_buffers_.begin();
  std::advance(end, free_buffers_.size() - chunk_size);

  // Free buffers contains CHUNB_SIZE chunks only.
  // There are no size header chunk.
  free_buffers_.erase_after_size(free_buffers_.cbefore_begin(), end,
                                 CHUNK_SIZE);

  assert(free_buffers_.size() == chunk_size);
}

void ChunkList::ReserveFreeChunk(size_t chunk_size)
{
  if (chunk_size <= free_buffers_.size()) {
    return;
  }

  size_t diff = chunk_size - free_buffers_.size();

  for (size_t i = 0; i < diff; ++i) {
    free_buffers_.push_back(free_buffers_.create_node_size(CHUNK_SIZE));
  }
}

auto ChunkList::ReserveWriteSpace(size_t size) -> void
{
  assert((CHUNK_SIZE & 0x1) == 0);

  for (auto const &buffer : buffers_) {
    if (size <= buffer.GetWritableSize()) return;
    size -= buffer.GetWritableSize();
  }

  assert(size >= 1);
  const auto chunk_size = ((size - 1) >> 12) + 1;
  ReserveFreeChunk(chunk_size);
}

auto ChunkList::GetFreeChunk() KANON_NOEXCEPT->ListType::Iterator
{
  if (!free_buffers_.empty()) {
    return free_buffers_.extract_front();
  } else {
    return free_buffers_.end();
  }
}

auto ChunkList::AppendChunkList(ChunkList *rhs) -> void
{
  buffers_.splice_after(buffers_.end(), rhs->buffers_);
}

auto ChunkList::DebugPrint() -> void
{
  puts("===== ChunkList debug print =====");
  printf("Buffer count = %zu\n", buffers_.size());
  printf("Free buffer count = %zu\n", free_buffers_.size());

  auto iter = buffers_.begin();
  if (!buffers_.empty()) {
    printf("Chunk Header info: %zu %zu %zu\n", (size_t)iter->read_index_,
           (size_t)iter->write_index_, (size_t)iter->GetMaxSize());
  }

  size_t i = 1;
  for (++iter; buffers_.end() != iter; ++iter, ++i) {
    printf("Chunk[%zu]: %zu %zu %zu\n", i, (size_t)iter->read_index_,
           (size_t)iter->write_index_, (size_t)iter->GetMaxSize());
  }
  puts("===== end ======");
}

auto ChunkList::AddChunk() -> Chunk *
{
  ListType::Iterator free_buffer;
  if (!free_buffers_.empty()) {
    DLOG("%s", "===== Reuse the free chunk =====\n");
    free_buffer = free_buffers_.extract_front();
    buffers_.push_back(free_buffer);
  } else {
    DLOG("%s", "===== New chunk =====\n");
    buffers_.push_back(buffers_.create_node_size(CHUNK_SIZE));
    free_buffer = buffers_.before_end();
  }

  DLOG("Free buffer: (%zu, %zu, %zu)\n", (size_t)free_buffer->read_index_,
       (size_t)free_buffer->write_index_, (size_t)free_buffer->GetMaxSize());
  if (buffers_.size() == 1) {
    DLOG("%s\n", "This is a header chunk");
    free_buffer->SetToHeader();
    ASSERT_HEADER;
  }

  return &*free_buffer;
}

} // namespace kanon
