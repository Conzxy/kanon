#ifndef KANON_UTIL_OPTIONAL_H
#define KANON_UTIL_OPTIONAL_H

// This is dummy optional.
// It is a wrapper of std::pair<bool, Result>
//
// before c++17, you can use boost::optional or absl::optional
// but I don't import them since this is just a base tool for some places
// and them are so huge and also have many dependence.
//
// In c++ 17, you can use std::optional
//
//
#include <type_traits>
#include <utility>
#include <assert.h>

#include "kanon/util/macro.h"

namespace kanon {
 
#if defined(CXX_STANDARD_17) && defined(USE_STD_OPTIONAL)
#include <optional>
  using std::optional;

template<typename T>
auto make_null_optional() {
  return std::optional<T>();
}
#else
 
// Don't like absl::optional
// It use option_data base class that
// including valid(bool), union of value and dummy
// we simplified this design 
template<typename T>
class optional;

namespace detail {

template<typename T>
struct is_optional_impl 
  : std::false_type {};

template<typename T>
struct is_optional_impl<optional<T>> 
  : std::true_type {};

template<typename T>
struct remove_cvref 
  : std::remove_cv<typename std::remove_reference<T>::type> {};

} // namespace detail

template<typename T>
struct is_optional 
  : detail::is_optional_impl<
  typename std::remove_cv<
    typename std::decay<T>
    ::type>
  ::type> {};

template<typename T> 
class optional {
  typedef optional<T> Self;
  typedef std::pair<bool, T> Rep;
public:
  template<typename = typename std::enable_if<
    std::is_default_constructible<T>::value
    >::type>
  optional() noexcept
    : opt_{ Rep{ false, T{} } }
  { }
  
  template<typename U = T, typename = typename std::enable_if<
    !is_optional<U>::value
  >::type>
  optional(U&& value) 
    : opt_{ Rep{ true, T{ std::forward<U>(value) } } }
  { }

  // compiler forwards to std::pair 
  // FIXME add type traits
  // Or may std::pair has provided?
  //
  template<typename U>
  optional(optional<U> const& other)
    : opt_{ Rep{ other.has_value(), other.value() } }
  { }

  template<typename U>
  optional(optional<U>&& other) noexcept
    : opt_{ Rep{ other.has_value(), std::move(other.value()) } }
  { }

  optional(optional const& other) = default;
  optional(optional&& other) noexcept = default;
  
  template<typename U>
  optional& operator=(optional<U> const& other) {
    opt_ = Rep{ other.has_value(), other.value() };

    return *this;
  }
  
  template<typename U>
  optional& operator=(optional<U>&& other) noexcept {
    std::swap(opt_, other.opt_);

    return *this;
  }

  optional& operator=(optional const& other) = default;
  optional& operator=(optional&& other) noexcept = default;

  template<typename U = T, typename = typename std::enable_if<
    !is_optional<U>::value
    >::type>
  optional& operator=(U&& other) {
    opt_ = Rep{ true, T{ std::forward<U>(other) } };
    return *this;
  }

  operator bool() const noexcept
  { return opt_.first; }

  bool has_value() const noexcept
  { return opt_.first; }
  
// if has_value() is false
// should throw bad_optional_access exception
  T const& value() const &
  { assert(has_value()); return opt_.second; }

  T& value() &
  { assert(has_value()); return opt_.second; }

  T const&& value() const &&
  { assert(has_value()); return std::move(opt_.second); }

  T&& value() &&
  { assert(has_value()); return std::move(opt_.second); }

  T const* operator->() const noexcept 
  { assert(has_value()); return std::addressof(opt_.second); }

  T* operator->() noexcept 
  { assert(has_value()); return std::addressof(opt_.second); }
  
  T const& operator*() const & noexcept
  { return value(); }

  T& operator*() & noexcept
  { return value(); }

  T const&& operator*() const && noexcept
  { return std::move(value()); }

  T&& operator*() && noexcept
  { return std::move(value()); }
  
  void SetInvalid() noexcept
  { opt_.first = false; }
private:
  std::pair<bool, T> opt_;
};

template<typename T>
KANON_CONSTEXPR kanon::optional<typename std::decay<T>::type>
make_optional(T&& val) {
  return kanon::optional<typename std::decay<T>::type>(std::forward<T>(val));
}

template<typename T>
KANON_CONSTEXPR kanon::optional<T>
make_null_optional() {
  return kanon::optional<T>{ };
}

#endif // end (cxx >= 17)
} // namespace kanon

#endif // KANON_UTIL_OPTIONAL_H
