#ifndef KANON_THREAD_ATOMIC_COUNTER_H
#define KANON_THREAD_ATOMIC_COUNTER_H

#include <atomic>
#include <type_traits>

#include "kanon/util/noncopyable.h"

namespace kanon {

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
class AtomicCounter : noncopyable {
  using Self = AtomicCounter;

 public:
  explicit AtomicCounter(T const init_val = 0) : count_(init_val) {}

  T operator=(T const val) noexcept {
    Set(val);
    return this->Val();
  }

  AtomicCounter(Self &&other) noexcept : count_(other.Val()) {}

  Self &operator=(Self &&other) noexcept {
    Set(other.Val());
    return *this;
  }

  T Reset() noexcept { count_.store(0, std::memory_order_relaxed); }

  void SetValue(T const val) noexcept { count_.store(val, std::memory_order_relaxed); }

  T GetValue() const noexcept { return count_.load(std::memory_order_relaxed); }

  T Add(T const val) noexcept { return count_.fetch_add(val, std::memory_order_relaxed) + val; }

  T GetAndAdd(T const val) noexcept { return count_.fetch_add(val, std::memory_order_relaxed); }

  T Sub(T const val) noexcept { return count_.fetch_sub(val, std::memory_order_relaxed) - val; }

  T GetAndSub(T const val) noexcept { return count_.fetch_sub(val, std::memory_order_relaxed); }

  operator T() const noexcept { return GetValue(); }
  T operator++() noexcept { return Add(1); }
  T operator++(int) noexcept { return GetAndAdd(1); }
  T operator--() noexcept { return Sub(1); }
  T operator--(int) noexcept { return GetAndSub(1); }

 private:
  std::atomic<T> count_;
};

using AtomicCounter32 = AtomicCounter<uint32_t>;
using AtomicCounter64 = AtomicCounter<uint64_t>;

}  // namespace kanon

#endif