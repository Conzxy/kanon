#pragma once
#ifndef FORWARD_LIST_FORWARD_LIST_ITERATOR_H
#define FORWARD_LIST_FORWARD_LIST_ITERATOR_H

#include "node_def.h"
#include "iterator.h"

namespace zstl {

/**
 * ForwardListIterator is a node in fact.
 * It satisfy the requirements of Forward Iterator
 */
template<typename T>
struct ForwardListIterator : std::iterator<std::forward_iterator_tag, T> {
  using BaseNode     = forward_list_detail::BaseLinkedNode;
  using Node         = forward_list_detail::LinkedNode<T>;
  using ValueType    = T;
  using Ref          = T&;
  using ConstRef     = T const&;
  using Pointer      = T*;
  using ConstPointer = T const*;
  using Self         = ForwardListIterator;
  using Header       = forward_list_detail::Header;

  ForwardListIterator(BaseNode* node=nullptr) 
   : node_{ node }
  {
  }

  Ref& operator*() noexcept { return extract()->val; }
  ConstRef operator*() const noexcept { return extract()->val; }
  Pointer operator->() noexcept { return &(extract()->val); }
  ConstPointer operator->() const noexcept { return &(extract()->val); }

  Self& operator++() noexcept
  {
    node_ = node_->next;
    return *this;
  }

  Self operator++(int) noexcept
  {
    auto ret = node_;
    node_ = node_->next;
    return ret;
  }

  void set(Node* node) noexcept { node_ = node; }
  Node* extract() noexcept { return static_cast<Node*>(node_); }
  Node const* extract() const noexcept { return static_cast<Node const*>(node_); }
  BaseNode* extract_base() noexcept { return node_; }
  BaseNode const* extract_base() const noexcept { return node_; }

  Self next() const noexcept { return Self(node_->next); }
private:
  BaseNode* node_;
};

template<typename T>
struct ForwardListConstIterator : std::iterator<std::forward_iterator_tag, T> {
  using BaseNode     = forward_list_detail::BaseLinkedNode;
  using Node         = forward_list_detail::LinkedNode<T>;
  using ValueType    = T;
  using Ref          = T const&;
  using ConstRef     = T const&;
  using Pointer      = T const*;
  using ConstPointer = T const*;
  using Self         = ForwardListConstIterator;
  using Header       = forward_list_detail::Header;
  
  ForwardListConstIterator(ForwardListIterator<T> it)
   : node_(it.extract())
  {
  }

  ForwardListConstIterator(BaseNode const* node=nullptr) 
    : node_(const_cast<BaseNode*>(node))
  {
  }
  
  ForwardListIterator<T> to_iterator() const noexcept
  { return { node_ }; }

  Ref& operator*() noexcept { return extract()->val; }
  ConstRef operator*() const noexcept { return extract()->val; }
  Pointer operator->() noexcept { return &(extract()->val); }
  ConstPointer operator->() const noexcept { return &(extract()->val); }

  Self& operator++() noexcept
  {
    node_ = node_->next;
    return *this;
  }

  Self operator++(int) noexcept
  {
    auto ret = node_;
    node_ = node_->next;
    return ret;
  }

  void set(Node const* node) noexcept { node_ = node; }
  Node const* extract() const noexcept { return static_cast<Node const*>(node_); }
  BaseNode const* extract_base() const noexcept { return node_; }
  Self next() const noexcept { return Self(node_->next); }
private:
  BaseNode* node_;
};

template<typename T>
inline bool operator==(ForwardListConstIterator<T> x, ForwardListConstIterator<T> y) noexcept
{ return x.extract() == y.extract(); }

template<typename T>
inline bool operator==(ForwardListIterator<T> x, ForwardListConstIterator<T> y) noexcept
{ return x.extract() == y.extract(); }

template<typename T>
inline bool operator==(ForwardListConstIterator<T> x, ForwardListIterator<T> y) noexcept
{ return x.extract() == y.extract(); }

template<typename T>
inline bool operator==(ForwardListIterator<T> x, ForwardListIterator<T> y) noexcept
{ return x.extract() == y.extract(); }

template<typename T>
inline bool operator!=(ForwardListConstIterator<T> x, ForwardListConstIterator<T> y) noexcept
{ return !(x == y); }

template<typename T>
inline bool operator!=(ForwardListIterator<T> x, ForwardListConstIterator<T> y) noexcept
{ return !(x == y); }

template<typename T>
inline bool operator!=(ForwardListConstIterator<T> x, ForwardListIterator<T> y) noexcept
{ return !(x == y); }

template<typename T>
inline bool operator!=(ForwardListIterator<T> x, ForwardListIterator<T> y) noexcept
{ return !(x == y); }

} // namespace zstl

#endif // FORWARD_LIST_FORWARD_LIST_ITERATOR
