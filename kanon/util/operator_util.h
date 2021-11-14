#ifndef KANON_OPERATOR_UTIL_H
#define KANON_OPERATOR_UTIL_H

namespace kanon {

// CRTP: since need infomation of subclass
template<typename D>
struct equal_comparable {
  friend bool operator != (D const& lhs, D const& rhs) 
    noexcept(noexcept(lhs == rhs))
  { return !(lhs == rhs); }

};

template<typename D>
struct less_than_comparable {
  friend bool operator > (D const& lhs, D const& rhs) 
    noexcept(noexcept(rhs < lhs))
  { return rhs < lhs; }

  friend bool operator >= (D const& lhs, D const& rhs) 
    noexcept(noexcept(lhs < rhs))
  { return !(lhs < rhs); }

  friend bool operator <= (D const& lhs, D const& rhs) 
    noexcept(noexcept(rhs < lhs))
  { return !(rhs < lhs); }
};

} 
#endif // KANON_OPERATOR_UTIL_H
