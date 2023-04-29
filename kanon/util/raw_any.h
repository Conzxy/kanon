#ifndef KANON_UTIL_RAW_ANY_H_
#define KANON_UTIL_RAW_ANY_H_

#include <memory> // addressof()

#include "kanon/util/macro.h"
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
  using AmbiguousCond =
      zstl::negation<std::is_same<typename std::decay<T>::type, RawAny>>;

 public:
  RawAny() KANON_NOEXCEPT : obj_(nullptr) {}

  ~RawAny() KANON_NOEXCEPT {}

  RawAny(RawAny const &) = default;

  RawAny(RawAny &&rhs) KANON_NOEXCEPT : obj_(rhs.obj_) { rhs.obj_ = nullptr; }

  RawAny &operator=(RawAny const &) = default;
  RawAny &operator=(RawAny &&rhs) KANON_NOEXCEPT
  {
    if (&rhs != this) {
      std::swap(rhs.obj_, obj_);
    }
    return *this;
  }

  RawAny(void *obj) KANON_NOEXCEPT : obj_(obj) {}

  template <typename T, zstl::enable_if_t<AmbiguousCond<T>::value, char> = 0>
  RawAny(T &obj)
    : obj_(std::addressof(obj))
  {
  }

  template <typename T, zstl::enable_if_t<AmbiguousCond<T>::value, char> = 0>
  RawAny &operator=(T &obj) KANON_NOEXCEPT
  {
    obj_ = std::addressof(obj);
    return *this;
  }

  /**
   * In most case, this function isn't called.
   * Unless you cast the argument to void* to force call it.
   */
  RawAny &operator=(void *obj) KANON_NOEXCEPT
  {
    obj_ = obj;
    return *this;
  }

  bool empty() const KANON_NOEXCEPT { return obj_ == nullptr; }
  void clear() KANON_NOEXCEPT { obj_ = nullptr; }

  void swap(RawAny &rhs) KANON_NOEXCEPT { std::swap(obj_, rhs.obj_); }

  template <typename V>
  friend V *UnsafeAnyCast(RawAny from) KANON_NOEXCEPT;

  template <typename V>
  friend V UnsafeAnyCast2(RawAny from) KANON_NOEXCEPT;

 private:
  void *obj_;
};

/* To RawAny, the cast must be unsafe, so the SafeAnyCast() and
 * AnyCast() is a wrapper of UnsafeAnyCast(). */

template <typename V>
KANON_INLINE V *UnsafeAnyCast(RawAny from) KANON_NOEXCEPT
{
  return reinterpret_cast<V *>(from.obj_);
}

template <typename V>
KANON_INLINE V *SafeAnyCast(RawAny from) KANON_NOEXCEPT
{
  return UnsafeAnyCast<V>(from);
}

/**
 * \param from The RawAny source
 * \tparam V The casted destination type must be a Object type
 * \return A pointer to Object type
 *
 * \warning
 *  The return type and V is different.
 *  If you don't like this, you should use \ref AnyCast2<V>()
 */
template <typename V>
KANON_INLINE V *AnyCast(RawAny from) KANON_NOEXCEPT
{
  return UnsafeAnyCast<V>(from);
}

template <typename V>
KANON_INLINE V UnsafeAnyCast2(RawAny from) KANON_NOEXCEPT
{
  static_assert(std::is_pointer<V>::value,
                "The template argument of AnyCast<> must be a pointer type");
  return reinterpret_cast<V>(from.obj_);
}

template <typename V>
KANON_INLINE V SafeAnyCast2(RawAny from) KANON_NOEXCEPT
{
  return UnsafeAnyCast2<V>(from);
}

/**
 * The replacement of AnyCast<V>().
 * The return type is same with the template parament
 */
template <typename V>
KANON_INLINE V AnyCast2(RawAny from) KANON_NOEXCEPT
{
  return UnsafeAnyCast2<V>(from);
}

} // namespace kanon

#endif // KANON_UTIL_RAW_ANY_H_
