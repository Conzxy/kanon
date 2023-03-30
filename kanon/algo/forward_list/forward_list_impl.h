#ifndef STL_SUP_FORWARD_LIST_IMPL_H
#define STL_SUP_FORWARD_LIST_IMPL_H

#ifndef STL_SUP_FORWARD_LIST_H
#include "../forward_list.h"
#endif

#include <algorithm>
#include <utility>

#ifdef FORWARD_LIST_DEBUG
#include <iostream>
#endif

#include "algorithm.h"
#include "kanon/util/macro.h"

#define FORWARD_LIST_TEMPLATE_LIST template <typename T, typename A>
#define FORWARD_LIST_TEMPLATE      ForwardList<T, A>

namespace zstl {

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::resize(SizeType n, ValueType const &val)
{
  if (size() <= n) {
    auto diff = n - size();

    while (diff--) {
      push_back(val);
    }
  } else {
    auto it = zstl::advance_iter(before_begin(), n);

    while (it.next() != end()) {
      erase_after(it);
    }
  }
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::assign(SizeType count, ValueType const &value)
{
  Iterator beg;
  for (beg = before_begin(); count != 0 && beg.next() != end(); ++beg) {
    AllocTraits::destroy(*this, &*beg.next());
    AllocTraits::construct(*this, &*beg.next(), value);
    --count;
  }

  // count > size()
  if (count > 0) {
    while (count--) {
      push_front(value);
    }
  } else if (count < 0) {
    // count < size()
    while (beg.next() != end()) {
      erase_after(beg);
    }
  }
}

FORWARD_LIST_TEMPLATE_LIST
template <typename InputIterator, typename>
void FORWARD_LIST_TEMPLATE::assign(InputIterator first, InputIterator last)
{
  Iterator beg;
  for (beg = before_begin(); beg.next() != end() && first != last;
       ++beg, ++first)
  {
    AllocTraits::destroy(*this, &*beg.next());
    AllocTraits::construct(*this, &*beg.next(), *first);
  }

  // size() < distance(first, last)
  if (beg.next() == end()) {
    while (first != last) {
      // insert_after(beg, *first);
      push_back(*first++);
    }
  } else if (first == last) {
    // size() > distance(first, last)
    while (beg.next() != end()) {
      erase_after(beg);
    }
  }
}

FORWARD_LIST_TEMPLATE_LIST
auto FORWARD_LIST_TEMPLATE::operator=(Self const &other) -> Self &
{
  assign(other.begin(), other.end());
  return *this;
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::push_front(Node *new_node) KANON_NOEXCEPT
{
  forward_list_detail::push_front(header_, new_node);
  ++header_->count;
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::push_back(Node *new_node) KANON_NOEXCEPT
{
  forward_list_detail::push_back(header_, new_node);
  ++header_->count;
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::insert_after(ConstIterator pos, Node *new_node)
{
  auto node = pos.to_iterator().extract();
  forward_list_detail::insert_after(header_, node, new_node);

  ++header_->count;
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::insert_after(ConstIterator pos, SizeType count,
                                         ValueType const &val)
{
  // At first, I want create a list whose length is count
  // the first node do some work like insert_after()
  // And update header_->prev in term of pos
  // But it is easy to make fault.

  // The push_back() is so simple, thus not wrong.
  while (count--) {
    insert_after(pos, create_node(val));
    ++pos;
  }
}

FORWARD_LIST_TEMPLATE_LIST
template <typename InputIterator, typename>
void FORWARD_LIST_TEMPLATE::insert_after(ConstIterator pos, InputIterator first,
                                         InputIterator last)
{
  for (; first != last; ++first) {
    // push_back(create_node(*first));
    insert_after(pos, create_node(*first));
    ++pos;
  }
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::pop_front()
{
  auto node = extract_front_node();
  drop_node(node);
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::pop_front_size(size_t sz)
{
  auto node = extract_front_node();
  drop_node_size(node, sz);
}

FORWARD_LIST_TEMPLATE_LIST
auto FORWARD_LIST_TEMPLATE::extract_front_node() KANON_NOEXCEPT->Node *
{
  --header_->count;
  return static_cast<Node *>(forward_list_detail::extract_front(header_));
}

FORWARD_LIST_TEMPLATE_LIST
auto FORWARD_LIST_TEMPLATE::erase_after(ConstIterator pos) -> Iterator
{
  drop_node(extract_after_node(pos));
  return pos.next().to_iterator();
}

FORWARD_LIST_TEMPLATE_LIST
auto FORWARD_LIST_TEMPLATE::erase_after_size(ConstIterator pos, size_t sz)
    -> Iterator
{
  drop_node_size(extract_after_node(pos), sz);
  return pos.next().to_iterator();
}

FORWARD_LIST_TEMPLATE_LIST
auto FORWARD_LIST_TEMPLATE::extract_after_node(ConstIterator pos)
    KANON_NOEXCEPT->Node *
{
  --header_->count;
  return static_cast<Node *>(
      forward_list_detail::extract_after(header_, pos.to_iterator().extract()));
}

FORWARD_LIST_TEMPLATE_LIST
auto FORWARD_LIST_TEMPLATE::erase_after(ConstIterator first, ConstIterator last)
    -> Iterator
{
  auto first_next = forward_list_detail::extract_after(
      header_, first.to_iterator().extract(), last.to_iterator().extract());

  BaseNode *old_next;

  // Drop all elements in (first, last)
  if (first_next != nullptr) {
    while (first_next != last.extract()) {
      old_next = first_next->next;
      drop_node(static_cast<Node *>(first_next));
      first_next = old_next;
      --header_->count;
    }
  }

  return last.to_iterator();
}

FORWARD_LIST_TEMPLATE_LIST
auto FORWARD_LIST_TEMPLATE::erase_after_size(ConstIterator first,
                                             ConstIterator last, size_t sz)
    -> Iterator
{
  auto first_next = forward_list_detail::extract_after(
      header_, first.to_iterator().extract(), last.to_iterator().extract());

  BaseNode *old_next;

  // Drop all elements in (first, last)
  if (first_next != nullptr) {
    while (first_next != last.extract()) {
      old_next = first_next->next;
      drop_node_size(static_cast<Node *>(first_next), sz);
      first_next = old_next;
      --header_->count;
    }
  }

  return last.to_iterator();
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::clear() KANON_NOEXCEPT
{
  if (!header_) {
    return;
  }

  while (header_->next) {
    auto node = header_->next;
    header_->next = node->next;
    drop_node(node);
  }

  Base::reset();
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::clear_size(size_t sz) KANON_NOEXCEPT
{
  if (!header_) {
    return;
  }

  while (header_->next) {
    auto node = header_->next;
    header_->next = node->next;
    drop_node_size(node, sz);
  }

  Base::reset();
}

FORWARD_LIST_TEMPLATE_LIST
template <typename DropCb>
void FORWARD_LIST_TEMPLATE::clear(DropCb cb) KANON_NOEXCEPT
{
  if (!header_) {
    return;
  }

  while (header_->next) {
    auto node = header_->next;
    header_->next = node->next;
    cb(node);
  }

  Base::reset();
}

FORWARD_LIST_TEMPLATE_LIST
template <typename UnaryPred>
auto FORWARD_LIST_TEMPLATE::search_before(UnaryPred pred, ConstIterator pos)
    -> Iterator
{
  if (pos == end()) return end();

  for (auto beg = pos.extract_base(); beg->next != nullptr; beg = beg->next) {
    if (pred(GET_LINKED_NODE_VALUE(beg->next))) {
      return Iterator(const_cast<BaseNode *>(beg));
    }
  }

  return end();
}

FORWARD_LIST_TEMPLATE_LIST
template <typename BinaryPred>
void FORWARD_LIST_TEMPLATE::merge(Self &list, BinaryPred pred)
{
  if (list.empty()) return;

  BaseNode *beg = header_;
  BaseNode *obeg = list.header_;

  while (beg->next != nullptr && obeg->next != nullptr) {
    if (pred(GET_LINKED_NODE_VALUE(obeg->next),
             GET_LINKED_NODE_VALUE(beg->next)))
    {
      auto old_node = obeg->next->next;
      insert_after(beg, list.extract_after(obeg));
      beg = beg->next;
      obeg->next = old_node;
    } else {
      beg = beg->next;
    }
  }

  if (beg->next == nullptr) {
    // It's also ok when list is empty
    splice_after(before_end(), list);
  }

  assert(list.empty());
  list.reset();
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::splice_after(ConstIterator pos, Self &list,
                                         ConstIterator it)
{
  insert_after(pos, list.extract_after(it));
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::splice_after(ConstIterator pos, Self &list,
                                         ConstIterator first,
                                         ConstIterator last)
{
  // (first, last)
  while (first.next() != last) {
    insert_after(pos, list.extract_after(first));
  }
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::splice_after(ConstIterator pos, Self &list)
{
  // If list is empty,
  // it is wrong to update prev.
  if (list.empty()) return;

  auto pos_node = pos.to_iterator().extract();
  auto old_next = pos_node->next;

  if (pos == before_end()) {
    header_->prev = list.before_end().extract();
  }

  pos_node->next = list.begin().extract();
  list.before_end().extract()->next = old_next;
  header_->count += list.size();

  list.reset();
  KANON_ASSERT(list.empty(), "The list must be empty");
}

FORWARD_LIST_TEMPLATE_LIST
template <typename UnaryPred>
auto FORWARD_LIST_TEMPLATE::remove_if(UnaryPred pred) -> SizeType
{
  SizeType count = 0;

  auto it = search_before(pred);
  while (it != end()) {
    ++count;
    erase_after(it);
    it = search_before(pred, it);
  }

  return count;
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::reverse() KANON_NOEXCEPT
{
  if (size() < 2) return;
  if (size() == 2) {
    push_back(extract_front());
    return;
  }

  forward_list_detail::reverse(header_);
}

FORWARD_LIST_TEMPLATE_LIST
template <typename BinaryPred>
auto FORWARD_LIST_TEMPLATE::unique(BinaryPred pred) -> SizeType
{
  SizeType count = 0;
  for (BaseNode *beg = header_;
       beg->next != nullptr && beg->next->next != nullptr;)
  {
    // Adjacent node with same value
    if (pred(GET_LINKED_NODE_VALUE(beg->next),
             GET_LINKED_NODE_VALUE(beg->next->next)))
    {
      erase_after(beg->next);
      ++count;
    } else {
      beg = beg->next;
      if (beg == nullptr) break;
    }
  }

  return count;
}

FORWARD_LIST_TEMPLATE_LIST
template <typename Compare>
void FORWARD_LIST_TEMPLATE::sort(Compare cmp)
{
  // TODO Optimization
  // lists store non-header list
  // To small list, maybe better a little.
  // It is not significant for large list.
  Self lists[64];
  Self list;
  int8_t end_of_lists = 0;

  while (!empty()) {
    list.push_front(extract_front());

    for (int8_t i = 0;; ++i) {
      if (lists[i].empty()) {
        list.swap(lists[i]);
        if (i == end_of_lists) end_of_lists++;
        break;
      } else {
        // Merge non-empty list
        // larger list merge shorter list
        if (lists[i].size() > list.size()) {
          lists[i].merge(list, cmp);
          list.swap(lists[i]);
        } else
          list.merge(lists[i], cmp);
      }
    }
  }

  assert(list.empty());

  for (int i = end_of_lists - 1; i >= 0; --i) {
    list.merge(lists[i], cmp);
  }

  *this = std::move(list);
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::sort2()
{

  ForwardList less;
  ForwardList equal;
  ForwardList larger;

  if (size() < 2) {
    return;
  }

  if (size() == 2) {
    if (!(*begin() < *begin().next())) {
      push_back(extract_front());
    }

    return;
  }

  auto pivot = *begin();
  Node *tmp;

  equal.push_back(extract_front());

  while (!empty()) {
    tmp = extract_front_node();
    if (tmp->val < pivot) {
      less.push_back(tmp);
    } else if (pivot < tmp->val) {
      larger.push_back(tmp);
    } else {
      equal.push_back(tmp);
    }
  }

  less.sort();
  larger.sort();

  less.splice_after(less.cbefore_end(), equal);
  less.splice_after(less.cbefore_end(), larger);

  *this = std::move(less);
}

FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::swap(Self &other) KANON_NOEXCEPT
{
  std::swap(this->header_, other.header_);
}

FORWARD_LIST_TEMPLATE_LIST
KANON_INLINE void swap(ForwardList<T, A> const &x, ForwardList<T, A> const &y)
    KANON_NOEXCEPT_OP(KANON_NOEXCEPT_OP(x.swap(y)))
{
  x.swap(y);
}

#ifdef FORWARD_LIST_DEBUG
FORWARD_LIST_TEMPLATE_LIST
void FORWARD_LIST_TEMPLATE::print() const KANON_NOEXCEPT
{
  std::cout << "==== print forward_list ====\n";
  std::cout << "Header -> ";
  for (auto i = header_->next; i != nullptr; i = i->next) {
    std::cout << GET_LINKED_NODE_VALUE(i) << " -> ";
  }

  std::cout << "(NULL)\n";
  std::cout << "============================" << std::endl;
}
#endif

FORWARD_LIST_TEMPLATE_LIST
KANON_INLINE bool operator==(ForwardList<T, A> const &x,
                             ForwardList<T, A> const &y)
{
  return std::equal(x.begin(), x.end(), y.begin());
}

FORWARD_LIST_TEMPLATE_LIST
KANON_INLINE bool operator!=(ForwardList<T, A> const &x,
                             ForwardList<T, A> const &y)
{
  return !(x == y);
}

FORWARD_LIST_TEMPLATE_LIST
KANON_INLINE bool operator<(ForwardList<T, A> const &x,
                            ForwardList<T, A> const &y)
{
  return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}

FORWARD_LIST_TEMPLATE_LIST
KANON_INLINE bool operator>(ForwardList<T, A> const &x,
                            ForwardList<T, A> const &y)
{
  return y < x;
}

FORWARD_LIST_TEMPLATE_LIST
KANON_INLINE bool operator<=(ForwardList<T, A> const &x,
                             ForwardList<T, A> const &y)
{
  return !(x > y);
}

FORWARD_LIST_TEMPLATE_LIST
KANON_INLINE bool operator>=(ForwardList<T, A> const &x,
                             ForwardList<T, A> const &y)
{
  return !(x < y);
}

} // namespace zstl

#endif // STL_SUP_FORWARD_LIST_IMPL_H
