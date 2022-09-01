#include "chunk_list.h"

#include <sys/uio.h>
#include <vector>
#include <limits.h>
#include <inttypes.h>

#include "kanon/net/sock_api.h"
#include "kanon/algo/fixed_vector.h"

#include "kanon/util/macro.h"

#ifdef IOV_MAX
  static constexpr unsigned IOVEC_MAX = IOV_MAX;
#else
  static constexpr unsigned IOVEC_MAX = 1024;
#endif

// #define CHUNK_LIST_DEBUG

#ifdef CHUNK_LIST_DEBUG
  #include <stdio.h>

  #define DLOG(str, ...) \
    ::printf(str, __VA_ARGS__)
#else
  #define DLOG(str, ...) 
#endif

static size_t RoundUpDivideIn2(size_t x, size_t y) noexcept {
  return (x == 0) ? 0 : ((x - 1) / y + 1);
}

namespace kanon {

void ChunkList::Append(void const *data, size_t len) {
  auto buf = reinterpret_cast<char const*>(data);

  // If buffers_ is not empty, try fill the the last chunk
  if (!buffers_.empty()) {
    DLOG("%s", "===== append to last chunk =====\n");
    auto last_block = buffers_.before_end();
    auto last_block_wsize = last_block->GetWritableSize();

    DLOG("Last chunk writable size = %zu\n", last_block_wsize);
    if (last_block_wsize > 0) {
      if (last_block_wsize >= len) {
        last_block->Append(buf, len);
        assert(last_block->GetWritableSize() == last_block_wsize-len);
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
      free_buffer->Append(buf, len);
      return; 
    }
  }
  KANON_ASSERT(GetLastChunk()->GetWritableSize() != CHUNK_SIZE, "The last chunk must not be empty");
}

void ChunkList::AdvanceRead(size_t len) {
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
        buffers_.drop_node(buffers_.extract_front_node());
      }

      len -= rsize;
    } else {
      first_block->AdvanceRead(len);
      return;
    }
  }
}

auto ChunkList::GetReadableSize() const noexcept -> SizeType {
  if (IsEmpty()) {
    return 0;
  } else {
    auto first_chunk = buffers_.begin();
    if (buffers_.size() == 1) {
      return first_chunk->GetReadableSize();
    } else if (buffers_.size() == 2) {
      return first_chunk->GetReadableSize() + first_chunk.next()->GetReadableSize();
    }

    auto last_chunk = buffers_.before_end();
    return first_chunk->GetMaxSize() != 4096 ? 
      first_chunk->GetReadableSize()+first_chunk.next()->GetReadableSize()+(GetChunkSize()-3)*CHUNK_SIZE + last_chunk->GetReadableSize():
      first_chunk->GetReadableSize()+(GetChunkSize()-2)*CHUNK_SIZE + last_chunk->GetReadableSize();
  }
}

ChunkList::SizeType ChunkList::WriteFd(int fd, int& saved_errno) noexcept {
  if (GetChunkSize() == 1) {
    DLOG("%s", "Chunk size = 1, call ::write() directly\n");
    auto first_chunk = GetFirstChunk();
    return sock::Write(fd, GetFirstChunk()->GetReadBegin(), first_chunk->GetReadableSize());
  }

  FixedVector<struct iovec> iovecs(GetChunkSize());
  auto first_chunk = buffers_.begin();
  auto cnt = RoundUpDivideIn2(GetChunkSize(), IOVEC_MAX);

  int ret = 0;

  DLOG("%s\n", "===== Write iovecs =====");
  while (cnt--) {
    for (auto& x : iovecs) {
      DLOG("readbegin = %p, readablesize = %zu\n", first_chunk->GetReadBegin(), first_chunk->GetReadableSize());
      x.iov_base = first_chunk->GetReadBegin();
      x.iov_len = first_chunk->GetReadableSize();
      ++first_chunk;
    }

    auto n = ::writev(fd, iovecs.data(), iovecs.size());

    if (n >= 0) {
      ret += n;
    } else {
      saved_errno = errno;
      break;
    }
  }

  return ret;
}

bool ChunkList::PutToFreeChunk() noexcept {
  if (buffers_.front().GetMaxSize() != CHUNK_SIZE) {
    return false;
  }

  DLOG("%s\n", "Put a new free chunk");
  auto free_buf = buffers_.extract_front();
  free_buf->Reset();
  free_buffers_.push_front(free_buf);
  return true;
}

void ChunkList::Shrink(size_t chunk_size) {
  if (chunk_size >= free_buffers_.size()) { return; }

  auto end = free_buffers_.begin();
  std::advance(end, free_buffers_.size()-chunk_size);

  free_buffers_.erase_after(free_buffers_.cbefore_begin(), end);
  assert(free_buffers_.size() == chunk_size);
}

void ChunkList::ReserveFreeChunk(size_t chunk_size) {
  if (chunk_size <= free_buffers_.size()) { return; }

  size_t diff = chunk_size - free_buffers_.size();

  for (size_t i = 0; i < diff; ++i) {
    free_buffers_.emplace_front();
  }
}

void ChunkList::ReserveWriteChunk(size_t chunk_size) {
  while (chunk_size--)
    buffers_.push_back(buffers_.create_node_size(CHUNK_SIZE));
}

auto ChunkList::GetFreeChunk() noexcept -> ListType::Iterator {
  if (!free_buffers_.empty()) {
    return free_buffers_.extract_front();
  } else {
    return free_buffers_.end();
  }
}

} // namespace kanon

