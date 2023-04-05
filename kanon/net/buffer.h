#ifndef _KANON_NET_BUFFER_H_
#define _KANON_NET_BUFFER_H_

#include "kanon/net/type.h"
#include "callback.h"
#include "kanon/buffer/buffer.h"
#include "kanon/util/arithmetic_type.h"

namespace kanon {

/**
 * \brief Read contents from @p fd and put them to buffer
 * \param fd File descriptor that must be a socket
 * \param saved_errno Save the errno that returned by ::read()
 *
 * \note
 *   This is a internal API, user don't care this.\n
 *   Then saved_errno is necessary since we don't process the error here.
 */
KANON_NET_NO_API usize BufferReadFromFd(Buffer &buffer, FdType fd,
                                        int &saved_errno);

KANON_NET_NO_API void BufferOverlapRecv(Buffer &buffer, FdType fd,
                                        int &saved_errno, void *overlap);
} // namespace kanon
#endif // _KANON_NET_BUFFER_H_
