#include "Item.h"

namespace memcache {

std::atomic<u64> Item::s_cas_(0); 

} // namespace memcache
