#include "kanon/thread/atomic.h"

namespace kanon {

#if defined(__GUNC__)
template class Atomic<int32_t>;
template class Atomic<int64_t>;
#endif

} // namespace kanon
