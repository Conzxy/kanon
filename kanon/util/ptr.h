#ifndef KANON_UTIL_UNIQUE_PTR_H
#define KANON_UTIL_UNIQUE_PTR_H

#include <memory>
#include <type_traits>
#include <assert.h>
#include <utility> // std:move()

#include "kanon/util/macro.h"

namespace kanon {

#if defined(CXX_STANDARD_11) && !defined(CXX_STANDARD_14)

template <typename T>
struct Is_unbounded_array : std::false_type {};

template <typename T>
struct Is_unbounded_array<T[]> : std::true_type {};

template <typename T, typename... Args,
          typename std::enable_if<!std::is_array<T>::value, char>::type = 0>
KANON_INLINE std::unique_ptr<T> make_unique(Args &&...args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T,
          typename std::enable_if<Is_unbounded_array<T>::value, int>::type = 0>
KANON_INLINE std::unique_ptr<T> make_unique(size_t num)
{
  return std::unique_ptr<T>(new typename std::remove_extent<T>::type[num]);
}

#elif defined(CXX_STANDARD_14)
using std::make_unique;
#endif

// compatible with smart_pointer and raw pointer
template <typename T, typename D>
KANON_INLINE T *GetPointer(std::unique_ptr<T, D> const &ptr) KANON_NOEXCEPT
{
  return ptr.get();
}

template <typename T>
KANON_INLINE T *GetPointer(std::shared_ptr<T> const &ptr) KANON_NOEXCEPT
{
  return ptr.get();
}

template <typename T>
KANON_INLINE T *GetPointer(T *const ptr) KANON_NOEXCEPT
{
  return ptr;
}

template <typename T>
struct KeyDeleter {
 public:
  explicit KeyDeleter(bool is_delete = true)
    : is_delete_(is_delete)
  {
  }

  void operator()(T *ptr) const
  {
    if (is_delete_) {
      delete ptr;
    }
  }

 private:
  bool is_delete_;
};

template <typename T>
using KeyUniquePtr = std::unique_ptr<T, KeyDeleter<T>>;

template <typename T>
KeyUniquePtr<T> MakeUniquePtrAsKey(T *p) KANON_NOEXCEPT
{
  return KeyUniquePtr<T>(p, KeyDeleter<T>(false));
}

// down_pointer_cast is hinted by google
// * In Debug mode, use dynamic_cast<> to check down_cast if is valid,
//   then use unsafe cast return casted pointer
// * In release mode, don't care, since we has checked in debug mode

//! Only allowed used for assigning or initializing a variable.
//! If you want use it as a temp object, Please use this:
//! down_pointer_cast(ptr.get()).
template <typename T, typename F, typename D>
KANON_INLINE std::unique_ptr<T, D> down_pointer_cast(std::unique_ptr<F, D> &ptr)
{
#ifndef NDEBUG
  assert(ptr != nullptr && dynamic_cast<T *>(ptr.get()) != nullptr);
#endif
  return std::unique_ptr<T>(reinterpret_cast<T *>(ptr.release()));
}

template <typename T, typename F>
KANON_INLINE std::shared_ptr<T> down_pointer_cast(std::shared_ptr<F> &ptr)
{
#ifndef NDEBUG
  assert(ptr != nullptr && std::dynamic_pointer_cast<T>(ptr) != nullptr);
#endif
  return std::static_pointer_cast<T>(ptr);
}

template <typename T, typename F>
KANON_INLINE T *down_pointer_cast(F *ptr)
{
#ifndef NDEBUG
  assert(ptr != nullptr && dynamic_cast<T *>(ptr) != nullptr);
#endif
  return reinterpret_cast<T *>(ptr);
}

template <typename T>
using DeferDelete = std::unique_ptr<T>;

template <typename T, typename D>
using DeferDelete2 = std::unique_ptr<T, D>;

/**
 * A convenient macro to define a dummy object 
 * that used for defering delete.
 *
 * User no need to declare and define a actual object 
 * to defer delete.
 * If no such macros, user may writes:
 * ```cpp
 * DeferDelete<T> xxxx(obj);
 * auto dter = [](T* obj) { ... };
 * DeferDelete<T, decltype(dter)> xxxx(obj, dter);
 * ```
 * These are unnecessary and ugly code I think.
 *
 * \note
 *  Since the obj__ is unique identifier.
 *  (since C++ requires variable name must be unique in same scope).
 */
#define KANON_DEFER_DELETE(type__, obj__) \
  kanon::DeferDelete<type__> type__##__defer__dummy__##obj__{obj__}

#define KANON_DEFER_DELETE2(type__, obj__, dter__) \
  auto type__##_##obj__##deleter___ = dter__; \
  kanon::DeferDelete2<type__, decltype(type__##_##obj__##deleter___)> type__##__defer_dummy__##obj__{obj__, std::move(type__##_##obj__##deleter___)}

/**
 * std::make_shared requires the constructor of the object
 * must be public(i.e. accessed).
 * In fact, the allocator_traits<Alloc>::construct() requires it.
 * (YOU can discover this through checking the error message produced by
 * compiler)
 *
 * No matter any case, because the specfic implementation is not cross
 * compiler, we can't make them to be the friend of object.
 *
 * Use a empty class and derived from the desired class,
 * declares the constructor of it to be protected, then
 * derived class can access.
 *
 * To reuse the logic, use template and scoped class.
 * \see https://stackoverflow.com/a/56676533
 */
template <typename T, typename... Args>
KANON_INLINE std::shared_ptr<T> MakeSharedFromProtected(Args &&...args)
{
  struct ProtectedProxy : public T {
    /* Here, must declares the type of arguments be Args&&.
     * If arguments is a prvalue, the type of template argument
     * is T, therefore, it will be copied instead of moved
     */
    ProtectedProxy(Args &&...args)
      : T(std::forward<Args>(args)...)
    {
    }
  };

  return std::make_shared<ProtectedProxy>(std::forward<Args>(args)...);
}

template <typename T, typename Alloc, typename... Args>
KANON_INLINE std::shared_ptr<T> AllocateSharedFromProtected(Alloc const &alloc,
                                                            Args &&...args)
{
  struct ProtectedProxy : public T {
    /* Here, must declares the type of arguments be Args&&.
     * If arguments is a prvalue, the type of template argument
     * is T, therefore, it will be copied instead of moved
     */
    ProtectedProxy(Args &&...args)
      : T(std::forward<Args>(args)...)
    {
    }
  };

  return std::allocate_shared<ProtectedProxy>(alloc,
                                              std::forward<Args>(args)...);
}
} // namespace kanon

#endif
