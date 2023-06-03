#ifndef KANON_THREAD_ATOMIC_COUNTER_H
#define KANON_THREAD_ATOMIC_COUNTER_H

#include <atomic>
#include <type_traits>

#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"

namespace kanon {

template <typename T,
          typename = typename std::enable_if<std::is_integral<T>::value>::type>
class AtomicCounter : noncopyable {
  using Self = AtomicCounter;

 public:
  explicit AtomicCounter(T const init_val = 0)
    : count_(init_val)
  {
  }

  T operator=(T const val) KANON_NOEXCEPT
  {
    SetValue(val);
    return this->GetValue();
  }

  AtomicCounter(Self &&other) KANON_NOEXCEPT : count_(other.GetValue()) {}

  Self &operator=(Self &&other) KANON_NOEXCEPT
  {
    SetValue(other.GetValue());
    return *this;
  }

  T Reset() KANON_NOEXCEPT { count_.store(0, std::memory_order_relaxed); }

  void SetValue(T const val) KANON_NOEXCEPT
  {
    count_.store(val, std::memory_order_relaxed);
  }

  T GetValue() const KANON_NOEXCEPT
  {
    return count_.load(std::memory_order_relaxed);
  }

  T Add(T const val) KANON_NOEXCEPT
  {
    return count_.fetch_add(val, std::memory_order_relaxed) + val;
  }

  T GetAndAdd(T const val) KANON_NOEXCEPT
  {
    return count_.fetch_add(val, std::memory_order_relaxed);
  }

  T Sub(T const val) KANON_NOEXCEPT
  {
    return count_.fetch_sub(val, std::memory_order_relaxed) - val;
  }

  T GetAndSub(T const val) KANON_NOEXCEPT
  {
    return count_.fetch_sub(val, std::memory_order_relaxed);
  }

  operator T() const KANON_NOEXCEPT { return GetValue(); }
  T operator++() KANON_NOEXCEPT { return Add(1); }
  T operator++(int) KANON_NOEXCEPT { return GetAndAdd(1); }
  T operator--() KANON_NOEXCEPT { return Sub(1); }
  T operator--(int) KANON_NOEXCEPT { return GetAndSub(1); }

 private:
  std::atomic<T> count_;
};

using AtomicCounter32 = AtomicCounter<uint32_t>;
using AtomicCounter64 = AtomicCounter<uint64_t>;

} // namespace kanon

#endif
