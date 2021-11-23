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
 * @class Any
 * @brief 
 * accept any type object and store it in this object(Sure, it is constructed in heap)
 * @note 
 * The class should not be a class template.
 * Since we may want it can do:
 * std::vector<Any> instead of std::vector<Any<T>>.
 * In this case, it need template argumnt deduction but T must be a specific type.
 *
 * To achieve this purpose, we use a technique "type erasure" to erase the template parameter.
 */
class Any {
public:
  Any() = default;

  // If V is Any&, says that argument type must be Any&, it should call Any(Any const&)
  template<typename V,
    typename std::enable_if<!std::is_same<Any&, V>::value, char>::type = 0>
  Any(V&& val)
  // Store the non-qualified type(If is function or array, also should store the decayed type)
    : holder_{ new Holder<typename std::remove_cv<
      typename std::decay<V>::type
        >::type>{ std::forward<V>(val) } }
  { }
  
  Any(Any const& rhs)
    : holder_{ rhs.empty() ? nullptr : rhs.holder_->clone(rhs.holder_) }
  { }
  
  Any(Any&& rhs) KANON_NOEXCEPT
    : holder_{ rhs.holder_ }
  {
    rhs.holder_ = nullptr;
  }
  
  template<typename V>
  Any& operator=(V&& v) {
    Any(v).swap(v);

    return *this;
  }

  Any& operator=(Any const& rhs) {
    Any(rhs).swap(*this);

    return *this;
  }

  Any& operator=(Any&& rhs) KANON_NOEXCEPT {
    this->swap(rhs);

    return *this;
  }
  
  ~Any() KANON_NOEXCEPT {
    clear();
  }

  bool empty() const KANON_NOEXCEPT 
  { return holder_ == nullptr; }

  void clear() {
    if (holder_) 
      delete holder_;
    holder_ = nullptr;
  }
  
  std::type_info const& type() const {
    return holder_->type();
  }  

  void swap(Any& rhs) KANON_NOEXCEPT {
    std::swap(holder_, rhs.holder_);
  }

private:
  /**
   * use friend function to hide the implemetation detail
   * parameter use * instead of & to determine the argument if valid
   * 
   */
  template<typename V>
  friend V* AnyCast(Any* from);

  template<typename V>
  friend V* unsafeAnyCast(Any* from);

  /**
   * @class HolderBase
   * @brief 
   * HolderBase is a dummy base class.
   * Use it we can erase the type from Holder<>.
   * Also, to call the correct function from HolderBase*,
   * we need declare (pure) virtual function.
   * This is the unavoidable cost of "type erase" technique.
   * @see Holder further
   */
  class HolderBase {
  public:
    HolderBase() = default;

    // Althought it is not necessary here
    virtual ~HolderBase() KANON_NOEXCEPT = default; 

    /**
     * @brief 
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
   * @tparam V the type of value which we want to store
   * @brief 
   * This is actual implemetation class
   * it store the value and implematation some useful interface from its base class
   */
  template<typename V>
  class Holder final
    : public HolderBase {
  public:
    Holder() = default;
  
    ~Holder() KANON_NOEXCEPT = default;

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
  char const* what() const KANON_NOEXCEPT KANON_OVERRIDE {
    return "BadAnyCast: Any object maybe not initialized or "
      "type not match";
  }
};

template<typename V>
inline V* AnyCast(Any* from) {
  // If V is reference type, we don't remove it
  // and trigger a error.
  typedef typename std::remove_cv<V>::type NonCV;

  return (from && from->type() == typeid(V)) ?
    std::addressof(static_cast<Any::Holder<NonCV>*>(from->holder_)->holder_) :
    nullptr;

}

template<typename V>
inline V const* AnyCast(Any const* from) {
  return AnyCast<V>(const_cast<Any*>(from));
}

// In this case, we can't know referenece if valid
template<typename V>
inline V AnyCast(Any& from) {
  typedef typename std::remove_reference<V>::type NonRef;

  auto result = AnyCast<NonRef>(std::addressof(from));

  if (!result) {
    throw BadAnyCastException{};
  }
  
  /* 
  typedef typename std::conditional<
    std::is_reference<V>::value,
    V,
    typename std::add_lvalue_reference<V>::type
  >::type RefV;
  
  // static_cast<T&> don't call copy constructor
  // and *result is not local, the cast must be safe.
  return static_cast<RefV>(*result);
  */ 

  // But you think carefully, 
  // following one line is same effect,
  // so I don't know why boost do it...
  return *result;
}

template<typename V>
inline V* unsafeAnyCast(Any* from) {
  return from ? 
    std::addressof(static_cast<Any::Holder<V>*>(from->holder_)->holder_) :
    nullptr;
}

template<typename V>
inline V const* unsafeAnyCast(Any const* from) {
  return unsafeAnyCast<V>(const_cast<V*>(from));
}

} // namespace kanon

#endif // KANON_ANY_H
