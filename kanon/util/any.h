#ifndef KANON_ANY_H
#define KANON_ANY_H

#include <type_traits>
#include <utility> // forward, move, swap
#include <assert.h>
#include <typeinfo> // typeid
#include <exception> // bad_cast

#include "kanon/util/macro.h"

namespace kanon {

/**
 * Accept any type object and store it in this object(Sure, it is constructed in heap)
 */
class Any final {
public:
  Any() 
    : holder_(nullptr)
  {
  }

  template<typename V,
    typename std::enable_if<!std::is_same<Any, typename std::decay<V>::type>::value, char>::type = 0>
  Any(V&& val)
  { 
    static_assert(std::is_copy_constructible<typename std::decay<V>::type>::value,
      "The type of given argument must be CopyConstructible");
    holder_ = new Holder<typename std::decay<V>::type>{ std::forward<V>(val) };
  }
  
  Any(Any const& rhs)
    : holder_{ rhs.empty() ? nullptr : rhs.holder_->clone(rhs.holder_) }
  { }
  
  Any(Any&& rhs) noexcept
    : holder_{ rhs.holder_ }
  {
    rhs.holder_ = nullptr;
  }
  
  template<typename V>
  Any& operator=(V&& v) {
    Any(std::forward<V>(v)).swap(*this);
    return *this;
  }

  Any& operator=(Any const& rhs) {
    Any(rhs).swap(*this);

    return *this;
  }

  Any& operator=(Any&& rhs) noexcept {
    this->swap(rhs);

    return *this;
  }
  
  ~Any() noexcept {
    clear();
  }

  bool empty() const noexcept 
  { return holder_ == nullptr; }

  void clear() {
    if (holder_) 
      delete holder_;
    holder_ = nullptr;
  }
  
  std::type_info const& type() const {
    return holder_->type();
  }  

  void swap(Any& rhs) noexcept {
    std::swap(holder_, rhs.holder_);
  }

private:
  /**
   * use friend function to hide the implemetation detail
   * parameter use * instead of & to determine the argument if valid
   * 
   */
  template<typename V>
  friend V* SafeAnyCast(Any const& from);

  template<typename V>
  friend V* UnsafeAnyCast(Any const& from);

  /**
   * @class HolderBase
   * \brief 
   * HolderBase is a dummy base class.
   * Use it we can erase the type from Holder<>.
   * Also, to call the correct function from HolderBase*,
   * we need declare (pure) virtual function.
   * This is the unavoidable cost of "type erase" technique.
   * \see Holder further
   */
  class HolderBase {
  public:
    HolderBase() = default;

    // Althought it is not necessary here
    virtual ~HolderBase() noexcept = default; 

    /**
     * \brief 
     * Since we can't get static type from HolderBase,
     * we need RTTI(run time type identify) to determine type cast if safe
     */ 
    virtual std::type_info const& type() const = 0;
    
    /**
     * @berif 
     * Since HolderBase have not value field
     */
    virtual HolderBase* clone(HolderBase* holder) = 0;
  };

  /**
   * @class Holder
   * \tparam V the type of value which we want to store
   * \brief 
   * This is actual implemetation class
   * it store the value and implematation some useful interface from its base class
   */
  template<typename V>
  class Holder final
    : public HolderBase {
  public:
    Holder() = default;
  
    ~Holder() noexcept = default;

    Holder(V const& v)
      : holder_{ v }
    { }

    Holder(V&& v)
      : holder_{ std::move(v) }
    { }
    
    std::type_info const& type() const KANON_OVERRIDE {
      return typeid(V);
    }

    Holder* clone(HolderBase* holder) KANON_OVERRIDE {
      assert(holder);
      return new Holder{ *static_cast<Holder<V>*>(holder) };
    }

    V holder_;
  };

  HolderBase* holder_;
};

class BadAnyCastException : public std::bad_cast {
public:
  char const* what() const noexcept KANON_OVERRIDE {
    return "BadAnyCast: Any object maybe not initialized or "
           "type not match";
  }
};

template<typename V>
inline V* SafeAnyCast(Any const& from) {
  // If V is reference type, we don't remove it
  // and trigger a error.
  static_assert(!std::is_reference<V>::value,
    "The type to cast must not be reference");

  if (from.type() == typeid(V)) {
    return std::addressof(static_cast<Any::Holder<V>*>(from.holder_)->holder_); 
  }

  return nullptr;
}


// inline V SafeAnyCast(Any const& from) {
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

template<typename V>
inline V* UnsafeAnyCast(Any const& from) {
  static_assert(!std::is_reference<V>::value,
    "The type to cast must not be reference");

  return std::addressof(static_cast<Any::Holder<V>*>(from.holder_)->holder_);
}

// template<typename V>
// inline V const* UnsafeAnyCast(Any const* from) {
//   return UnsafeAnyCast<V>(const_cast<V*>(from));
// }

template<typename V>
inline V* AnyCast(Any const& from) {
#ifndef NDEBUG
  return SafeAnyCast<V>(from);
#else
  return UnsafeAnyCast<V>(from);
#endif
}

} // namespace kanon

#endif // KANON_ANY_H