#ifndef KANON_ANY_H
#define KANON_ANY_H

#include <type_traits>
#include <utility> // forward, move, swap
#include <assert.h>
#include <typeinfo>  // typeid
#include <exception> // bad_cast

#include "kanon/util/macro.h"

namespace kanon {

/**
 * Accept any type object and store it in this object(Sure, it is constructed in
 * heap)
 *
 * \warning
 *  The object must be copyable.
 *  If you want to store move-only object, should use `UniqueAny`(\see
 * unique_any.h)
 */
class Any final {
 public:
  Any()
    : holder_(nullptr)
  {
  }

  /*
   * The reason why `Any` requires the object must be copyable is that the
   * instantiation of Holder<> will also instantiate the clone() member
   * function. Then clone() will detect whether the copy member of the object is
   * deleted(disabled).
   *
   * YOU may confuse, because the instantiation of the member function of class
   * template is lazy(or implicit), why clone() definition is instantiated
   * although it is not called ever?
   *
   * Notice, the clone() is a virtual function, the virtual call mechanism
   * requires the virtual function actually exists as a linkable entity(i.e. as
   * a entry in the virtual table)
   *
   * \see <<C++ templates 2nd>> 14.2.2
   */
  template <typename V,
            typename std::enable_if<
                !std::is_same<Any, typename std::decay<V>::type>::value,
                char>::type = 0>
  Any(V &&val)
  {
    static_assert(
        std::is_copy_constructible<typename std::decay<V>::type>::value,
        "The type of given argument must be CopyConstructible");
    holder_ = new Holder<typename std::decay<V>::type>{std::forward<V>(val)};
  }

  Any(Any const &rhs)
    : holder_{rhs.empty() ? nullptr : rhs.holder_->clone()}
  {
  }

  Any(Any &&rhs) KANON_NOEXCEPT : holder_{rhs.holder_}
  {
    rhs.holder_ = nullptr;
  }

  template <typename V>
  Any &operator=(V &&v)
  {
    Any(std::forward<V>(v)).swap(*this);
    return *this;
  }

  Any &operator=(Any const &rhs)
  {
    Any(rhs).swap(*this);

    return *this;
  }

  Any &operator=(Any &&rhs) KANON_NOEXCEPT
  {
    this->swap(rhs);

    return *this;
  }

  ~Any() KANON_NOEXCEPT { clear(); }

  bool empty() const KANON_NOEXCEPT { return holder_ == nullptr; }

  void clear()
  {
    if (holder_) delete holder_;
    holder_ = nullptr;
  }

  std::type_info const &type() const { return holder_->type(); }

  void swap(Any &rhs) KANON_NOEXCEPT { std::swap(holder_, rhs.holder_); }

 private:
  /**
   * use friend function to hide the implemetation detail
   * parameter use * instead of & to determine the argument if valid
   *
   */
  template <typename V>
  friend V *SafeAnyCast(Any const &from);

  template <typename V>
  friend V *UnsafeAnyCast(Any const &from);

  /**
   * \brief
   * HolderBase is a dummy base class.
   * Use it we can erase the type from Holder<>.
   * Also, to call the correct function from HolderBase*,
   * we need declare (pure) virtual function.
   * This is the unavoidable cost of "type erase" technique.
   * \see Holder further
   */
  class KANON_CORE_API HolderBase {
   public:
    HolderBase() = default;

    // Althought it is not necessary here
    virtual ~HolderBase() KANON_NOEXCEPT = default;

    /**
     * \brief
     * Since we can't get static type from HolderBase,
     * we need RTTI(run time type identify) to determine type cast if safe
     */
    virtual std::type_info const &type() const = 0;

    /**
     * \berif
     * Since HolderBase have not value field
     */
    virtual HolderBase *clone() = 0;
  };

  /**
   * \tparam V the type of value which we want to store
   * \brief
   * This is actual implemetation class
   * it store the value and implematation some useful interface from its base
   * class
   */
  template <typename V>
  class Holder final : public HolderBase {
   public:
    Holder() = default;

    /* Don't mark KANON_NOEXCEPT */
    ~Holder() = default;

    Holder(V const &v)
      : holder_{v}
    {
    }

    Holder &operator=(Holder const &o) { holder_ = o; }

    /* Don't mark KANON_NOEXCEPT */
    Holder(V &&v)
      : holder_{std::move(v)}
    {
    }

    Holder &operator=(Holder &&o) { holder_ = std::move(o); }

    std::type_info const &type() const KANON_OVERRIDE { return typeid(V); }

    Holder *clone() KANON_OVERRIDE { return new Holder{holder_}; }

    V holder_;
  };

  HolderBase *holder_;
};

class BadAnyCastException : public std::bad_cast {
 public:
  char const *what() const KANON_NOEXCEPT KANON_OVERRIDE
  {
    return "BadAnyCast: Any object maybe not initialized or "
           "type not match";
  }
};

template <typename V>
KANON_INLINE V *SafeAnyCast(Any const &from)
{
  // If V is reference type, we don't remove it
  // and trigger a error.
  static_assert(!std::is_reference<V>::value,
                "The type to cast must not be reference");

  if (from.type() == typeid(V)) {
    return std::addressof(static_cast<Any::Holder<V> *>(from.holder_)->holder_);
  }

  return nullptr;
}

// KANON_INLINE V SafeAnyCast(Any const& from) {
//   typedef typename std::remove_reference<V>::type NonRef;

//   auto result = AnyCast<NonRef>(std::addressof(from));

//   if (!result) {
//     throw BadAnyCastException{};
//   }

//   /*
//   typedef typename std::conditional<
//     std::is_reference<V>::value,
//     V,
//     typename std::add_lvalue_reference<V>::type
//   >::type RefV;

//   // static_cast<T&> don't call copy constructor
//   // and *result is not local, the cast must be safe.
//   return static_cast<RefV>(*result);
//   */

//   // But you think carefully,
//   // following one line is same effect,
//   // so I don't know why boost do it...
//   return *result;
// }

template <typename V>
KANON_INLINE V *UnsafeAnyCast(Any const &from)
{
  static_assert(!std::is_reference<V>::value,
                "The type to cast must not be reference");

  return std::addressof(static_cast<Any::Holder<V> *>(from.holder_)->holder_);
}

// template<typename V>
// KANON_INLINE V const* UnsafeAnyCast(Any const* from) {
//   return UnsafeAnyCast<V>(const_cast<V*>(from));
// }

template <typename V>
KANON_INLINE V *AnyCast(Any const &from)
{
#ifndef NDEBUG
  return SafeAnyCast<V>(from);
#else
  return UnsafeAnyCast<V>(from);
#endif
}

} // namespace kanon

#endif // KANON_ANY_H
