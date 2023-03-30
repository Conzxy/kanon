#ifndef FORWARD_LIST_FORWARD_LIST_ITERATOR_H
#define FORWARD_LIST_FORWARD_LIST_ITERATOR_H

#include "node_def.h"
#include "kanon/zstl/iterator.h"

namespace zstl {

/**
 * ForwardListIterator is a node in fact.
 * It satisfy the requirements of Forward Iterator
 */
template <typename T>
struct ForwardListIterator {
  using BaseNode = forward_list_detail::BaseLinkedNode;
  using Node = forward_list_detail::LinkedNode<T>;
  using ValueType = T;
  using Ref = T &;
  using ConstRef = T const &;
  using Pointer = T *;
  using ConstPointer = T const *;
  using Self = ForwardListIterator;
  using Header = forward_list_detail::Header;

  using pointer = T *;
  using const_pointer = T const *;
  using value_type = T;
  using reference = T &;
  using const_reference = T const &;
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;

  ForwardListIterator(BaseNode *node = nullptr)
    : node_{node}
  {
  }

  Ref &operator*() KANON_NOEXCEPT { return extract()->val; }
  ConstRef operator*() const KANON_NOEXCEPT { return extract()->val; }
  Pointer operator->() KANON_NOEXCEPT { return &(extract()->val); }
  ConstPointer operator->() const KANON_NOEXCEPT { return &(extract()->val); }

  Self &operator++() KANON_NOEXCEPT
  {
    node_ = node_->next;
    return *this;
  }

  Self operator++(int) KANON_NOEXCEPT
  {
    auto ret = node_;
    node_ = node_->next;
    return ret;
  }

  void set(Node *node) KANON_NOEXCEPT { node_ = node; }
  Node *extract() KANON_NOEXCEPT { return static_cast<Node *>(node_); }
  Node const *extract() const KANON_NOEXCEPT
  {
    return static_cast<Node const *>(node_);
  }
  BaseNode *extract_base() KANON_NOEXCEPT { return node_; }
  BaseNode const *extract_base() const KANON_NOEXCEPT { return node_; }

  Self next() const KANON_NOEXCEPT { return Self(node_->next); }

 private:
  BaseNode *node_;
};

template <typename T>
struct ForwardListConstIterator {
  using BaseNode = forward_list_detail::BaseLinkedNode;
  using Node = forward_list_detail::LinkedNode<T>;
  using ValueType = T;
  using Ref = T const &;
  using ConstRef = T const &;
  using Pointer = T const *;
  using ConstPointer = T const *;
  using Self = ForwardListConstIterator;
  using Header = forward_list_detail::Header;

  using pointer = T const *;
  using const_pointer = T const *;
  using value_type = T;
  using reference = T const &;
  using const_reference = T const &;
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;

  ForwardListConstIterator(ForwardListIterator<T> it)
    : node_(it.extract())
  {
  }

  ForwardListConstIterator(BaseNode const *node = nullptr)
    : node_(const_cast<BaseNode *>(node))
  {
  }

  ForwardListIterator<T> to_iterator() const KANON_NOEXCEPT { return {node_}; }

  Ref &operator*() KANON_NOEXCEPT { return extract()->val; }
  ConstRef operator*() const KANON_NOEXCEPT { return extract()->val; }
  Pointer operator->() KANON_NOEXCEPT { return &(extract()->val); }
  ConstPointer operator->() const KANON_NOEXCEPT { return &(extract()->val); }

  Self &operator++() KANON_NOEXCEPT
  {
    node_ = node_->next;
    return *this;
  }

  Self operator++(int) KANON_NOEXCEPT
  {
    auto ret = node_;
    node_ = node_->next;
    return ret;
  }

  void set(Node const *node) KANON_NOEXCEPT { node_ = node; }
  Node const *extract() const KANON_NOEXCEPT
  {
    return static_cast<Node const *>(node_);
  }
  BaseNode const *extract_base() const KANON_NOEXCEPT { return node_; }
  Self next() const KANON_NOEXCEPT { return Self(node_->next); }

 private:
  BaseNode *node_;
};

template <typename T>
KANON_INLINE bool operator==(ForwardListConstIterator<T> x,
                             ForwardListConstIterator<T> y) KANON_NOEXCEPT
{
  return x.extract() == y.extract();
}

template <typename T>
KANON_INLINE bool operator==(ForwardListIterator<T> x,
                             ForwardListConstIterator<T> y) KANON_NOEXCEPT
{
  return x.extract() == y.extract();
}

template <typename T>
KANON_INLINE bool operator==(ForwardListConstIterator<T> x,
                             ForwardListIterator<T> y) KANON_NOEXCEPT
{
  return x.extract() == y.extract();
}

template <typename T>
KANON_INLINE bool operator==(ForwardListIterator<T> x,
                             ForwardListIterator<T> y) KANON_NOEXCEPT
{
  return x.extract() == y.extract();
}

template <typename T>
KANON_INLINE bool operator!=(ForwardListConstIterator<T> x,
                             ForwardListConstIterator<T> y) KANON_NOEXCEPT
{
  return !(x == y);
}

template <typename T>
KANON_INLINE bool operator!=(ForwardListIterator<T> x,
                             ForwardListConstIterator<T> y) KANON_NOEXCEPT
{
  return !(x == y);
}

template <typename T>
KANON_INLINE bool operator!=(ForwardListConstIterator<T> x,
                             ForwardListIterator<T> y) KANON_NOEXCEPT
{
  return !(x == y);
}

template <typename T>
KANON_INLINE bool operator!=(ForwardListIterator<T> x,
                             ForwardListIterator<T> y) KANON_NOEXCEPT
{
  return !(x == y);
}

} // namespace zstl

#endif // FORWARD_LIST_FORWARD_LIST_ITERATOR
