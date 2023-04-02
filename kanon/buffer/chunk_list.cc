#include "chunk_list.h"

#include <limits.h>
#include <inttypes.h>

#include "kanon/util/macro.h"

// #define CHUNK_LIST_DEBUG

#ifdef CHUNK_LIST_DEBUG
#  include <stdio.h>

#  define DLOG(str, ...) ::printf(str, __VA_ARGS__)
#else
#  define DLOG(str, ...)
#endif

namespace kanon {

ChunkList::~ChunkList() KANON_NOEXCEPT
{
  if (!free_buffers_.empty()) {
    auto first_size = free_buffers_.front().GetMaxSize();
    if (first_size < CHUNK_SIZE) buffers_.pop_front_size(first_size);
  }
  buffers_.clear_size(CHUNK_SIZE);
  free_buffers_.clear_size(CHUNK_SIZE);
}

void ChunkList::Append(void const *data, size_t len)
{
  auto buf = reinterpret_cast<char const *>(data);

  // If buffers_ is not empty, try fill the the last chunk
  if (!buffers_.empty()) {
    DLOG("%s", "===== append to last chunk =====\n");
    auto last_block = buffers_.before_end();
    auto last_block_wsize = last_block->GetWritableSize();

    DLOG("Last chunk writable size = %zu\n", last_block_wsize);
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

  ListType::Iterator free_buffer;

  DLOG("%s", "===== Appent to the back of buffers_ =====\n");
  while (len > 0) {
    if (!free_buffers_.empty()) {
      DLOG("%s", "===== Reuse the free chunk =====\n");
      free_buffer = free_buffers_.extract_front();
      buffers_.push_back(free_buffer);
    } else {
      DLOG("%s", "===== New chunk =====\n");
      buffers_.push_back(buffers_.create_node_size(CHUNK_SIZE));
      free_buffer = buffers_.before_end();
    }

    if (len >= CHUNK_SIZE) {
      free_buffer->Append(buf, CHUNK_SIZE);
      len -= CHUNK_SIZE;
      buf += CHUNK_SIZE;
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

  while (!IsEmpty()) {
    first_block = buffers_.begin();
    auto const rsize = first_block->GetReadableSize();
    DLOG("readable size = %zu\n", rsize);
    // To the size() == 1, we also put it to free_buffers_
    // then we can append content in CHUNK_SIZE at most
    // since there is only one avaliable chunk to use
    // reuse also can avoid to call ::malloc()
    if (len >= first_block->GetReadableSize()) {
      if (!PutToFreeChunk()) {
        assert(CHUNK_SIZE != buffers_.front().GetMaxSize());
        buffers_.drop_node_size(buffers_.extract_front_node(), CHUNK_SIZE);
      }

      len -= rsize;
    } else {
      first_block->AdvanceRead((Chunk::size_type)len);
      return;
    }
  }
}

auto ChunkList::GetReadableSize() const KANON_NOEXCEPT -> SizeType
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
    return first_chunk->GetMaxSize() != 4096
               ? first_chunk->GetReadableSize() +
                     first_chunk.next()->GetReadableSize() +
                     (GetChunkSize() - 3) * CHUNK_SIZE +
                     last_chunk->GetReadableSize()
               : first_chunk->GetReadableSize() +
                     (GetChunkSize() - 2) * CHUNK_SIZE +
                     last_chunk->GetReadableSize();
  }
}

bool ChunkList::PutToFreeChunk() KANON_NOEXCEPT
{
  if (buffers_.front().GetMaxSize() != CHUNK_SIZE) {
    return false;
  }

  DLOG("%s\n", "Put a new free chunk");
  auto free_buf = buffers_.extract_front();
  free_buf->Reset();
  free_buffers_.push_front(free_buf);
  return true;
}

void ChunkList::Shrink(size_t chunk_size)
{
  if (chunk_size >= free_buffers_.size()) {
    return;
  }

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

void ChunkList::ReserveWriteChunk(size_t chunk_size)
{
  while (chunk_size--)
    buffers_.push_back(buffers_.create_node_size(CHUNK_SIZE));
}

auto ChunkList::GetFreeChunk() KANON_NOEXCEPT -> ListType::Iterator
{
  if (!free_buffers_.empty()) {
    return free_buffers_.extract_front();
  } else {
    return free_buffers_.end();
  }
}

} // namespace kanon
