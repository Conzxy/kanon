#ifndef KANON_OPERATOR_UTIL_H
#define KANON_OPERATOR_UTIL_H

#include "kanon/util/macro.h"

namespace kanon {

// CRTP: since need infomation of subclass
template <typename D>
struct equal_comparable {
  friend bool operator!=(D const &lhs, D const &rhs)
      KANON_NOEXCEPT_OP(KANON_NOEXCEPT_OP(lhs == rhs))
  {
    return !(lhs == rhs);
  }
};

template <typename D>
struct less_than_comparable {
  friend bool operator>(D const &lhs, D const &rhs)
      KANON_NOEXCEPT_OP(KANON_NOEXCEPT_OP(rhs < lhs))
  {
    return rhs < lhs;
  }

  friend bool operator>=(D const &lhs, D const &rhs)
      KANON_NOEXCEPT_OP(KANON_NOEXCEPT_OP(lhs < rhs))
  {
    return !(lhs < rhs);
  }

  friend bool operator<=(D const &lhs, D const &rhs)
      KANON_NOEXCEPT_OP(KANON_NOEXCEPT_OP(rhs < lhs))
  {
    return !(rhs < lhs);
  }
};

} // namespace kanon

#endif // KANON_OPERATOR_UTIL_H
