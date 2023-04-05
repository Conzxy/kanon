#ifndef KANON_NET_CHUNK_LIST_H__
#define KANON_NET_CHUNK_LIST_H__

#include "kanon/net/type.h"
#include "kanon/buffer/chunk_list.h"

namespace kanon {

KANON_NET_NO_API ChunkList::SizeType
ChunkListWriteFd(ChunkList &buffer, FdType fd, int &saved_errno) KANON_NOEXCEPT;

KANON_NET_NO_API void ChunkListOverlapSend(ChunkList &buffer, FdType fd,
                                           int &saved_errno, void *overlap);

KANON_INLINE ChunkList::SizeType ChunkListWriteFd(ChunkList &buffer,
                                                  FdType fd) KANON_NOEXCEPT
{
  int saved_errno;
  return ChunkListWriteFd(buffer, fd, saved_errno);
}

} // namespace kanon

#endif