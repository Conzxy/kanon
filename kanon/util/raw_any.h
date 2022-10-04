#ifndef KANON_UTIL_RAW_ANY_H_
#define KANON_UTIL_RAW_ANY_H_

#include <memory> // addressof()
#include "kanon/zstl/type_traits.h"

namespace kanon {

/**
 * Due to the `UniqueAny` just move contents,
 * the address is changed, the binded `this`
 * of lambda is invalid.
 *
 * I think `RawAny` is better than them
 * if you can manage it properly.
 *
 * \warning
 *  `RawAny` don't manage the object
 *  i.e. YOU must free it manually
 */
class RawAny final {
  template <typename T>
  using AmbiguousCond = zstl::negation<std::is_same<typename std::decay<T>::type, RawAny>>;
 public:
  RawAny()
    : obj_(nullptr)
  {
  }

  template <typename T, 
           zstl::enable_if_t<AmbiguousCond<T>::value, char> = 0>
  RawAny(T &obj)
    : obj_(std::addressof(obj))
  {
  }

  template <typename T, 
           zstl::enable_if_t<AmbiguousCond<T>::value, char> = 0>
  RawAny &operator=(T &obj) noexcept
  {
    obj_ = std::addressof(obj);
    return *this;
  }

  bool empty() const noexcept { return obj_ == nullptr; }
  void clear() noexcept { obj_ = nullptr; }

  void swap(RawAny &rhs) noexcept { std::swap(obj_, rhs.obj_); }

  template <typename V>
  friend V *SafeAnyCast(RawAny from);

  template <typename V>
  friend V *UnsafeAnyCast(RawAny from);

 private:
  void *obj_;
};

template <typename V>
V *UnsafeAnyCast(RawAny from)
{
  return reinterpret_cast<V *>(from.obj_);
}

template <typename V>
V *SafeAnyCast(RawAny from)
{
  return UnsafeAnyCast<V>(from);
}

template <typename V>
V *AnyCast(RawAny from)
{
  return UnsafeAnyCast<V>(from);
}

} // namespace kanon

#endif // KANON_UTIL_RAW_ANY_H_