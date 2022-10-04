#ifndef KANON_UNIQUE_ANY_H_
#define KANON_UNIQUE_ANY_H_

#include <type_traits>
#include <utility>  // forward, move, swap
#include <assert.h>
#include <typeinfo> // typeid
#include <exception> // bad_cast
#include "kanon/util/macro.h"

namespace kanon {

/**
 * Due to `Any` don't supports move-only object
 * (it requires the object must be copyable)
 *
 * Removing some members related to copy of `Any` to make it supports
 * move-only object, the `UniqueAny` owns the object(unique semantic).
 */
class UniqueAny {
public:
  UniqueAny() 
    : holder_(nullptr)
  {
  }

  template<typename V,
    typename std::enable_if<!std::is_same<UniqueAny, typename std::decay<V>::type>::value, char>::type = 0>
  UniqueAny(V&& val)
  { 
    holder_ = new Holder<typename std::decay<V>::type>{ std::forward<V>(val) };
  }
  
  UniqueAny(UniqueAny const &) = delete;
  UniqueAny &operator=(UniqueAny const &) = delete;

  UniqueAny(UniqueAny&& rhs) noexcept
    : holder_{ rhs.holder_ }
  {
    rhs.holder_ = nullptr;
  }
  
  template<typename V>
  UniqueAny& operator=(V&& v) {
    UniqueAny(std::forward<V>(v)).swap(*this);
    return *this;
  }

  UniqueAny& operator=(UniqueAny&& rhs) noexcept {
    this->swap(rhs);
    return *this;
  }
  
  ~UniqueAny() noexcept {
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

  void swap(UniqueAny& rhs) noexcept {
    std::swap(holder_, rhs.holder_);
  }

private:
  /**
   * use friend function to hide the implemetation detail
   * parameter use * instead of & to determine the argument if valid
   * 
   */
  template<typename V>
  friend V* SafeUniqueAnyCast(UniqueAny const& from);

  template<typename V>
  friend V* UnsafeUniqueAnyCast(UniqueAny const& from);

  /**
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
    
  };

  /**
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
    ~Holder() = default;
  
    /* Disable copy */
    Holder(V const& v) = delete;
    Holder &operator=(Holder const &) = delete;

    Holder(V&& v)
      : holder_{ std::move(v) }
    { }
    
    Holder &operator=(V &&v)
    {
      holder_ = std::move(v);
    }

    std::type_info const& type() const KANON_OVERRIDE {
      return typeid(V);
    }

    V holder_;
  };

  HolderBase* holder_;
};

class BadUniqueAnyCastException : public std::bad_cast {
public:
  char const* what() const noexcept KANON_OVERRIDE {
    return "BadUniqueAnyCast: UniqueAny object maybe not initialized or "
           "type not match";
  }
};

template<typename V>
inline V* SafeUniqueAnyCast(UniqueAny const& from) {
  // If V is reference type, we don't remove it
  // and trigger a error.
  static_assert(!std::is_reference<V>::value,
    "The type to cast must not be reference");

  if (from.type() == typeid(V)) {
    return std::addressof(static_cast<UniqueAny::Holder<V>*>(from.holder_)->holder_); 
  }

  return nullptr;
}

/** For compatible */
template<typename V>
inline V* SafeAnyCast(UniqueAny const& from) {
  return SafeUniqueAnyCast<V>(from);
}

template<typename V>
inline V* UnsafeUniqueAnyCast(UniqueAny const& from) {
  static_assert(!std::is_reference<V>::value,
    "The type to cast must not be reference");

  return std::addressof(static_cast<UniqueAny::Holder<V>*>(from.holder_)->holder_);
}

/** For compatible */
template<typename V>
inline V* UnsafeAnyCast(UniqueAny const& from) {
  return UnsafeUniqueAnyCast<V>(from);
}

template<typename V>
inline V* UniqueAnyCast(UniqueAny const& from) {
#ifndef NDEBUG
  return SafeUniqueAnyCast<V>(from);
#else
  return UnsafeUniqueAnyCast<V>(from);
#endif
}

/** For compatible */
template<typename V>
inline V* AnyCast(UniqueAny const& from) {
  return UniqueAnyCast<V>(from);
}

} // namespace kanon

#endif // KANON_UNIQUE_ANY_H_
