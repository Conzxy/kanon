#ifndef _ZSTL_ITERATOR_H
#define _ZSTL_ITERATOR_H

#include <iterator>

#include "kanon/zstl/type_traits.h"
#include "kanon/util/macro.h"

namespace zstl {

// 必须通过iterator_traits访问
// 因为它对T*有特化
#define ITER_TYPE_EXTRACTOR(name, type)                                        \
  template <typename I>                                                        \
  using name = typename std::iterator_traits<I>::type;

//************************************************************
// template type alias for iterator accessing
// the nested type trait
//************************************************************
ITER_TYPE_EXTRACTOR(iter_value_t, value_type)
ITER_TYPE_EXTRACTOR(iter_reference_t, reference)
ITER_TYPE_EXTRACTOR(iter_pointer_t, pointer)
ITER_TYPE_EXTRACTOR(iter_difference_type_t, difference_type)
ITER_TYPE_EXTRACTOR(iter_iterator_category_t, iterator_category)

// STL虽然提供了move_if_noexcept，但没提供uninitialized_move_if_noexcept()也没有提供make_move_iterator_if_noexcept
// 当T为throw move & copyable constructible时返回左值引用（即拷贝语义）
// 否则，返回右值引用
//
// 针对UninitializedMoveIfNoexcept()，需求是constructible，故没有判断assignment
//
template <typename T>
struct MoveIfNoexcept
  : zstl::negation<
        zstl::conjunction<zstl::negation<std::is_nothrow_move_constructible<T>>,
                          std::is_copy_constructible<T>>> {};

template <typename Iter, typename Dummy = zstl::enable_if_t<
                             MoveIfNoexcept<iter_value_t<Iter>>::value>>
auto MakeMoveIteratorIfNoexcept_impl(Iter const &iter)
    KANON_NOEXCEPT->decltype(std::make_move_iterator(iter))
{
  return std::make_move_iterator(iter);
}

template <typename Iter, typename Dummy = zstl::enable_if_t<
                             !MoveIfNoexcept<iter_value_t<Iter>>::value>>
Iter MakeMoveIteratorIfNoexcept_impl(Iter const &iter) KANON_NOEXCEPT
{
  return iter;
}

template <typename Iter>
auto MakeMoveIteratorIfNoexcept(Iter const &iter)
    KANON_NOEXCEPT->decltype(MakeMoveIteratorIfNoexcept_impl(iter))
{
  return MakeMoveIteratorIfNoexcept_impl(iter);
}

#define MAKE_MOVE_ITERATOR_IF_NOEXCEPT(iter)                                   \
  MakeMoveIteratorIfNoexcept_impl(iter)

template <typename Iter>
using IterValueType = typename std::iterator_traits<Iter>::value_type;

template <typename Iter>
using IterRef = typename std::iterator_traits<Iter>::reference;

template <typename Iter>
using IterPointer = typename std::iterator_traits<Iter>::pointer;

template <typename Iter>
using IterCategory = typename std::iterator_traits<Iter>::iterator_category;

template <typename Iter>
using IterDiffType = typename std::iterator_traits<Iter>::difference_type;

namespace detail {

template <typename Iter, typename Tag, typename = zstl::void_t<>>
struct IsIteratorOf : std::false_type {};

template <typename Iter, typename Tag>
struct IsIteratorOf<Iter, Tag,
                    zstl::void_t<zstl::enable_if_t<
                        std::is_convertible<IterCategory<Iter>, Tag>::value>>>
  : std::true_type {};

} // namespace detail

template <typename Iter, typename Tag>
struct IsIteratorOf : detail::IsIteratorOf<Iter, Tag> {};

#define is_xxx_iterator(name, tag)                                             \
  template <typename Iter>                                                     \
  using is_##name##_iterator = typename IsIteratorOf<Iter, std::tag>::type

is_xxx_iterator(input, input_iterator_tag);
is_xxx_iterator(output, output_iterator_tag);
is_xxx_iterator(forward, forward_iterator_tag);
is_xxx_iterator(bidirectional, bidirectional_iterator_tag);
is_xxx_iterator(random_access, random_access_iterator_tag);

template <typename Iter>
struct is_iterator
  : zstl::bool_constant<is_input_iterator<Iter>::value ||
                        is_output_iterator<Iter>::value> {};

template <typename Iter>
ZSTL_CONSTEXPR zstl::enable_if_t<zstl::is_iterator<Iter>::value, Iter>
advance_iter(Iter it, zstl::IterDiffType<Iter> d)
{
  std::advance(it, d);
  return it;
}

} // namespace zstl

#endif // _ZSTL_ITERATOR_H
