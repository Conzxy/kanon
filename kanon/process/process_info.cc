#include "kanon/process/process_info.h"

using namespace kanon;

StringView 
process::pidString() KANON_NOEXCEPT {

  static char pid_cache[32];
  static bool has_cache = false;
  
  if (!has_cache) {
    
    if (::snprintf(pid_cache, sizeof pid_cache, 
          "%d", pid()) < 0) {
      ::fprintf(stderr, "cache pid failed: %s\n", ::strerror(errno));
    } 
  } 

  return pid_cache;
}

StringView
process::hostname() KANON_NOEXCEPT {
  // hostname is limited to HOST_NAME_MAX in posix,
  // it is 64 in linux.
  static char hostname_cache[64];
  static bool has_cache = false;
  
  if (!has_cache) {
    
    if (::gethostname(hostname_cache, sizeof hostname_cache) < 0) {
      ::fprintf(stderr, "cache pid failed: %s\n", ::strerror(errno));
    } 
  } 

  return hostname_cache;
}
