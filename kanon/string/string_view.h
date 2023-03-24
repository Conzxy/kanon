#ifndef KANON_STRING_VIEW_H
#define KANON_STRING_VIEW_H

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#include <assert.h>
#include <cstddef>
#include <stdexcept>
#include <string.h>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

#include "kanon/util/macro.h"
#include "strcasecmp.h"

namespace kanon {

/**
 * To be compatible with std::string and char const*.
 * You should use StringView if you need length
 */
class StringArg {
 public:
  StringArg(char const *str)
    : data_(str)
  {
  }

  StringArg(std::string const &str)
    : data_(str.c_str())
  {
  }

  constexpr char const *data() const noexcept { return data_; }

  operator char const *() const { return data_; }

 private:
  char const *data_;
};

/**
 * Only support std::string
 * because I just use it
 *
 * StringView just view which is read only
 * so the life time of string(or character) depend on user
 *
 * If you know Rust, this is like fat pointer
 * \see std::stringview(C+++17)
 */
class StringView {
 public:
  using size_type = unsigned long;
  using value_type = char;
  using difference_type = std::ptrdiff_t;
  using pointer = char *;
  using const_pointer = char const *;
  using reference = char &;
  using const_reference = char const &;
  using iterator = pointer;
  using const_iterator = const_pointer;

  // constructor
  StringView()
    : data_("")
    , len_(0)
  {
  }

  StringView(char const *str)
    : data_(str)
    , len_(strlen(str))
  {
  }

  constexpr StringView(char const *str, size_type len)
    : data_(str)
    , len_(len)
  {
  }

  // N3337 p292:
  // Overload resolution specify:
  // Excluding Lvalue transformation, and
  // Lvalue transformation includeing array-to-pointer
  // So if you call:
  // StringView s("hello");
  // It will call StringView(char const*) instead this
  // So this is deprecated!
  // You can use MakeStringView() to create a StringView for string literal
  // OR use *_sv to create it.
  // template<unsigned N>
  // StringView(char const(&literal)[N])
  //   : data_(literal), len_(N-1)
  // {
  // }

  StringView(std::string const &str)
    : data_(str.c_str())
    , len_(str.size())
  {
  }

  StringView(unsigned char const *str)
    : data_(reinterpret_cast<char const *>(str))
    , len_(strlen(data_))
  {
  }

  StringView(unsigned char const *str, size_type len)
    : data_(reinterpret_cast<char const *>(str))
    , len_(len)
  {
  }

  void reset()
  {
    data_ = "";
    len_ = 0;
  }

  void set(char const *data, size_type n)
  {
    data_ = data;
    len_ = n;
  }

  // wrong for char
  // because it may be prvalue

  // convert
  // constexpr operator char const*() const { return data_; }

  // capacity
  constexpr bool empty() const noexcept { return len_ == 0; }

  constexpr size_type size() const noexcept { return len_; }

  // position
  constexpr char const *begin() const noexcept { return data_; }

  constexpr char const *end() const noexcept { return data_ + len_; }

  // data access
  constexpr const_reference operator[](size_type n) const noexcept
  {
    return data_[n];
  }

  constexpr const_reference at(size_type n) const
  {
    return n >= len_ ? throw std::length_error{"Given index over or equal "
                                               "length of StringView"}
                     : data_[n];
  }

  constexpr const_reference front() const noexcept { return data_[0]; }

  constexpr const_reference back() const noexcept { return data_[len_ - 1]; }

  constexpr const_pointer data() const noexcept { return data_; }

  void swap(StringView &rhs) noexcept
  {
    std::swap(data_, rhs.data_);
    std::swap(len_, rhs.len_);
  }

  // modify operation
  KANON_CONSTEXPR void remove_prefix(size_type n) noexcept
  {
    data_ += n;
    len_ -= n;
  }

  KANON_CONSTEXPR void remove_suffix(size_type n) noexcept { len_ -= n; }

  KANON_INLINE size_type copy(char *dst, size_type count,
                              size_type pos = 0) const
  {
    if (pos >= len_) {
      throw std::out_of_range{"Given position over length of StringView"};
    }

    ::memcpy(dst, data_ + pos, count);
    return count;
  }

  /**
   * If @p count is not provided,
   * substr() return the remaining part
   */
  KANON_CONSTEXPR StringView substr(size_type pos = 0,
                                    size_type count = npos) const noexcept
  {
    auto len = std::min(count, len_ - pos);
    return StringView(data_ + pos, len);
  }

  KANON_CONSTEXPR StringView substr_range(size_type begin = 0,
                                          size_type end = npos) const noexcept
  {
    return substr(begin, end - begin);
  }

  size_type find(StringView v, size_type pos = 0) const noexcept
  {
    // Intead of kmp or other efficient algorithm?
    // It is maybe bring some overhead.

    // Ensure the len_ - pos not be a negative number
    KANON_ASSERT(pos <= len_, "The position argument should be [0, len)");

    if (v.size() <= len_) {
      size_type n = len_ - v.size() + 1;
      for (size_type i = pos; i < n; ++i) {
        if (!::strncmp(data_ + i, v.data(), v.size())) {
          return i;
        }
      }
    }

    return npos;
  }

  size_type find(char c, size_type pos = 0) const noexcept
  {
    for (size_type i = pos; i < len_; ++i) {
      if (c == data_[i]) return i;
    }

    return npos;
  }

  /**
   * The logical is different from the std::find_end
   * First check the sequence with the start of @p pos
   * then reverse find the @p v if match
   * @p pos the start position
   */
  size_type rfind(StringView v, size_type pos = npos) const noexcept
  {
    if (!v.empty()) {
      auto len = std::min(len_ - 1, pos);

      // Avoid --len == size_type(-1)
      for (;; --len) {
        if (data_[len] == v.front() && len_ - len >= v.size() &&
            memcmp(data_ + len, v.data(), v.size()) == 0)
          return len;

        if (len == 0) break;
      }
    }

    return npos;
  }

  size_type rfind(char c, size_type pos = npos) const noexcept
  {
    auto len = std::min(len_ - 1, pos);
    for (;; --len) {
      if (data_[len] == c) {
        return len;
      }

      if (len == 0) break;
    }

    return npos;
  }

  bool contains(StringView v) const noexcept { return find(v) != npos; }

  bool contains(char c) const noexcept { return find(c) != npos; }

  bool starts_with(StringView v) const noexcept
  {
    return (len_ >= v.size() && memcmp(data_, v.data(), v.size()) == 0) ? true
                                                                        : false;
  }

  bool starts_with(char c) const noexcept
  {
    return (len_ >= 1 && data_[0] == c) ? true : false;
  }

  bool ends_with(StringView v) const noexcept
  {
    return (len_ >= v.size() &&
            memcmp(data_ + len_ - v.size(), v.data(), v.size()) == 0)
               ? true
               : false;
  }

  bool ends_with(char c) const noexcept
  {
    return (len_ >= 1 && data_[len_ - 1] == c) ? true : false;
  }

  /**
   * Finds the first character equal to one of characters
   * in the given character sequence
   * \return
   * the index of the first occurrence of any character of the sequence,
   * if not found, return npos
   */
  size_type find_first_of(StringView v, size_type pos = 0) const noexcept
  {
    for (; pos < len_; ++pos) {
      if (charInRange(data_[pos], v)) return pos;
    }

    return npos;
  }

  size_type find_first_of(char c, size_type pos = 0) const noexcept
  {
    for (; pos < len_; ++pos)
      if (data_[pos] == c) return pos;

    return npos;
  }

  /**
   * Finds the last character equal to one of characters
   * in the given character sequence(i.e. character range)
   * \param pos the end position of the search range
   * \return
   * the index of the last occurrence of any character of the sequence,
   * if not found, return npos
   */
  size_type find_last_of(StringView v, size_type pos = npos) const noexcept
  {
    int i = static_cast<int>(std::min(len_ - 1, pos));

    for (; i >= 0; --i) {
      if (charInRange(data_[i], v)) return i;
    }

    return npos;
  }

  size_type find_last_of(char c, size_type pos = npos) const noexcept
  {
    int i = std::min(len_ - 1, pos);

    for (; i >= 0; --i)
      if (data_[i] == c) return pos;

    return npos;
  }

  size_type find_first_not_of(StringView v, size_type pos = 0) const noexcept
  {
    for (; pos < len_; ++pos) {
      if (!charInRange(data_[pos], v)) return pos;
    }
    return npos;
  }

  size_type find_first_not_of(char c, size_type pos = 0) const noexcept
  {
    for (; pos < len_; ++pos) {
      if (data_[pos] != c) return pos;
    }
    return npos;
  }

  size_type find_last_not_of(StringView v, size_type pos = npos) const noexcept
  {
    int i = static_cast<int>(std::min(len_ - 1, pos));

    for (; i >= 0; --i) {
      if (!charInRange(data_[i], v)) return i;
    }

    return npos;
  }

  size_type find_last_not_of(char c, size_type pos = npos) const noexcept
  {
    int i = static_cast<int>(std::min(len_ - 1, pos));

    for (; i >= 0; --i) {
      if (data_[i] == c) return i;
    }

    return npos;
  }

  int caseCompare(StringView v) const noexcept
  {
    int r = kanon::StrNCaseCompare(data_, v.data(),
                                   len_ < v.size() ? len_ : v.size());

    if (r == 0) {
      if (len_ < v.size())
        r = -1;
      else if (len_ > v.size())
        r = 1;
    }

    return r;
  }

  int caseCompare(char c) const noexcept
  {
    auto data_0 = ::toupper(data_[0]);
    auto _c = ::toupper(c);

    if (len_ < 1 || data_0 < _c) return -1;

    if (data_0 > _c) return 1;

    return (data_0 == c && len_ == 1) ? 0 : 1;
  }

  // lexicographic compare
  int compare(StringView v) const noexcept
  {
    int r = ::memcmp(data_, v.data(), len_ < v.size() ? len_ : v.size());

    if (r == 0) {
      if (len_ < v.size())
        r = -1;
      else if (len_ > v.size())
        r = 1;
    }

    return r;
  }

  int compare(char c) const noexcept
  {
    if (len_ < 1 || data_[0] < c) return -1;

    if (data_[0] > c) return 1;

    return (data_[0] == c && len_ == 1) ? 0 : 1;
  }

  std::vector<std::string> split(StringView spliter = " ") const;

  std::string ToString() const { return std::string{data(), size()}; }

 private:
  // helper
  // complexity O(sv.size())
  bool charInRange(char c, StringView const &sv) const noexcept
  {
    for (auto x : sv)
      if (c == x) return true;

    return false;
  }

 private:
  char const *data_;
  size_type len_;

  // static
  // as end indicator or error indicator
 public:
  static constexpr size_type npos = -1;
};

// This is only used for string literal
template <StringView::size_type N>
StringView MakeStringView(char const (&literal)[N])
{
  return StringView(static_cast<char const *>(literal), N - 1);
}

inline void swap(StringView &lhs,
                 StringView &rhs) noexcept(noexcept(lhs.swap(rhs)))
{
  lhs.swap(rhs);
}

inline bool operator==(StringView const &lhs, StringView const &rhs) noexcept
{
  return (lhs.size() == rhs.size()) &&
         memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

template <StringView::size_type N>
inline bool operator==(StringView const &lhs, char const (&rhs)[N]) noexcept
{
  return lhs == MakeStringView(rhs);
}

inline bool operator!=(StringView const &lhs, StringView const &rhs) noexcept
{
  return !(lhs == rhs);
}

template <StringView::size_type N>
inline bool operator!=(StringView const &lhs, char const (&rhs)[N]) noexcept
{
  return !(lhs == MakeStringView(rhs));
}

inline bool operator<(StringView const &lhs, StringView const &rhs) noexcept
{
  return lhs.compare(rhs) < 0;
}

template <StringView::size_type N>
inline bool operator<(StringView const &lhs, char const (&rhs)[N]) noexcept
{
  return lhs < MakeStringView(rhs);
}

inline bool operator>(StringView const &lhs, StringView const &rhs) noexcept
{
  return rhs < lhs;
}

template <StringView::size_type N>
inline bool operator>(StringView const &lhs, char const (&rhs)[N]) noexcept
{
  return rhs < lhs;
}

inline bool operator>=(StringView const &lhs, StringView const &rhs) noexcept
{
  return !(lhs < rhs);
}

template <StringView::size_type N>
inline bool operator>=(StringView const &lhs, char const (&rhs)[N]) noexcept
{
  return !(lhs < rhs);
}

inline bool operator<=(StringView const &lhs, StringView const &rhs) noexcept
{
  return !(lhs > rhs);
}

template <StringView::size_type N>
inline bool operator<=(StringView const &lhs, char const (&rhs)[N]) noexcept
{
  return !(lhs > rhs);
}

// template<typename Ostream>
// Ostream& operator<<(Ostream& os, StringView v) noexcept
// { return os << v.data(); }

namespace literal {
constexpr StringView operator""_sv(char const *str, std::size_t len) noexcept
{
  return StringView(str, len);
}

} // namespace literal

} // namespace kanon

#endif // KANON_STRING_VIEW_H
