#pragma once
#ifndef STL_SUP_ITERATOR_H
#define STL_SUP_ITERATOR_H

#include <iterator>

#include "macros.h"
#include "type_traits.h"

namespace zstl {

template<typename Iter>
using IterValueType = typename std::iterator_traits<Iter>::value_type;

template<typename Iter>
using IterRef = typename std::iterator_traits<Iter>::reference;

template<typename Iter>
using IterPointer = typename std::iterator_traits<Iter>::pointer;

template<typename Iter>
using IterCategory = typename std::iterator_traits<Iter>::iterator_category;

template<typename Iter>
using IterDiffType = typename std::iterator_traits<Iter>::difference_type;

namespace detail {

template<typename Iter, typename Tag, typename=zstl::void_t<>>
struct IsIteratorOf : std::false_type {};

template<typename Iter, typename Tag> 
struct IsIteratorOf < Iter, Tag, zstl::void_t<
  zstl::enable_if_t<std::is_convertible<IterCategory<Iter>, Tag>::value>
  >>
  : std::true_type {};

} // namespace detail

template<typename Iter, typename Tag>
struct IsIteratorOf : detail::IsIteratorOf<Iter, Tag> {};

#define is_xxx_iterator(name, tag) \
  template<typename Iter> \
  using is_##name##_iterator = typename IsIteratorOf<Iter, std::tag>::type

is_xxx_iterator(input, input_iterator_tag);
is_xxx_iterator(output, output_iterator_tag);
is_xxx_iterator(forward, forward_iterator_tag);
is_xxx_iterator(bidirectional, bidirectional_iterator_tag);
is_xxx_iterator(random_access, random_access_iterator_tag);

template<typename Iter>
struct is_iterator : zstl::bool_constant<
  is_input_iterator<Iter>::value ||
  is_output_iterator<Iter>::value> { };

template<typename Iter>
ZSTL_CONSTEXPR zstl::enable_if_t<zstl::is_iterator<Iter>::value, Iter>
advance_iter(Iter it, zstl::IterDiffType<Iter> d)
{
  std::advance(it, d);
  return it;
}

} // namespace zstl

#endif // STL_SUP_ITERATOR_H
