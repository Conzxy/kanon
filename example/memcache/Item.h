#ifndef KANON_EXAMPLE_MEMCACHE_ITEM_H
#define KANON_EXAMPLE_MEMCACHE_ITEM_H

#include "kanon/util/noncopyable.h"
#include "kanon/string/string_view.h"
#include "type.h"

#include <boost/container_hash/hash.hpp>

#include <stdint.h>
#include <atomic>

namespace memcache {

using namespace kanon;

class Item : noncopyable {
public:
  Item(StringView key,
       uint valLen,
       i64 flag,
       i32 expTime,
       u64 cas = 0)
    : keyLen_{ static_cast<uint>(key.size()) }
    , valLen_{ valLen }
    , flag_{ flag }
    , expTime_{ expTime }
    , cas_{ cas }
    , data_{ nullptr }
  {
    data_ = reinterpret_cast<char*>(::malloc(keyLen_+valLen_));
    assert(data_);
    append(key.data(), key.size());
  }
  
  ~Item() KANON_NOEXCEPT {
    ::free(data_);
  }
  
  void append(void const* data, usize sz, usize idx=0) {
    // !unsafe
    // safe append should be ensured by session
    assert(sz <= totalLen());
    ::memcpy(data_+idx, data, sz);
  }
  
  void append(kanon::StringView sv, usize idx=0) {
    append(sv.data(), sv.size(), idx);
  } 

  void appendValue(kanon::StringView sv, usize idx=0) {
    append(sv.data(), sv.size(), keyLen_ + idx);
  }

  void resetKey(StringView key) {
    assert(key.size() <= 250);
    if (key.size() > totalLen()) {
      data_ = reinterpret_cast<char*>(::realloc(data_, key.size()));
    }

    keyLen_ = key.size();
    //valLen_ = 2;

    ::memcpy(data_, key.data(), key.size());
    //::memcpy(data_ + key.size(), "\r\n", sizeof "\r\n");
  } 

  uint totalLen() const KANON_NOEXCEPT { return keyLen_ + valLen_; }
  
  uint valueLen() const KANON_NOEXCEPT {
    return valLen_;
  }

  uint keyLen() const KANON_NOEXCEPT {
    return keyLen_;
  }

  i64 flag() const KANON_NOEXCEPT { return flag_; }

  i32 expirationTime() const KANON_NOEXCEPT { return expTime_; }

  u64 cas() const KANON_NOEXCEPT { return cas_; }
  
  void incrCas() KANON_NOEXCEPT { cas_ = ++s_cas_; }
  // This may be not used
  void decrCas() KANON_NOEXCEPT { cas_ = --s_cas_; }
  
  u64 hash() const KANON_NOEXCEPT 
  { return boost::hash_range(data_, data_+totalLen()); }

  StringView key() const KANON_NOEXCEPT { return StringView{ data_, keyLen_ }; }
  StringView value() const KANON_NOEXCEPT { return StringView{ data_+keyLen_, valLen_ }; }
  
  char const* valueData() const KANON_NOEXCEPT { return data_+keyLen_; }
  
  bool endWithCRLF() const KANON_NOEXCEPT {
    auto len = totalLen();
    return data_[len-1] == '\n' && data_[len-2] == '\r';
  }  

private:
  uint keyLen_;
  uint valLen_;

  i64 flag_; 
  i32 expTime_;
  u64 cas_;
  static std::atomic<u64> s_cas_;
  char* data_;
};

} // namespace memcache

#endif // KANON_EXAMPLE_MEMCACHE_H