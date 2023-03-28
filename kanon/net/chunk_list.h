#ifndef KANON_NET_CHUNK_LIST_H__
#define KANON_NET_CHUNK_LIST_H__

#include "kanon/buffer/chunk_list.h"
#include "kanon/net/type.h"

namespace kanon {

ChunkList::SizeType ChunkListWriteFd(ChunkList &buffer, FdType fd,
                                     int &saved_errno) noexcept;

KANON_INLINE ChunkList::SizeType ChunkListWriteFd(ChunkList &buffer,
                                                  FdType fd) noexcept
{
  int saved_errno;
  return ChunkListWriteFd(buffer, fd, saved_errno);
}

void ChunkListOverlapSend(ChunkList &buffer, FdType fd, int &saved_errno,
                          void *overlap);
} // namespace kanon

#endif