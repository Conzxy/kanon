#ifndef STL_SUP_FORWARD_LIST_H
#define STL_SUP_FORWARD_LIST_H

#include <memory>
#include <assert.h>

#include "kanon/zstl/iterator.h"
#include "kanon/zstl/type_traits.h"

#include "forward_list/node_def.h"
#include "forward_list/forward_list_iterator.h"
#include "forward_list/algorithm.h"

namespace zstl {
namespace forward_list_detail {

template <typename T, typename Alloc>
using NodeAllocator = typename Alloc::template rebind<LinkedNode<T>>::other;

template <typename Alloc>
using HeaderAllocator = typename Alloc::template rebind<Header>::other;

/**
 * EBCO
 * Need two allocator: header and node
 * This class also manage the header allocate and deallocate
 * but don't construct and destroy it, it just a POD object.
 */
template <typename T, typename Alloc = std::allocator<T>>
class ForwardListBase
  : protected NodeAllocator<T, Alloc>
  , protected HeaderAllocator<Alloc> {
 private:
  using HeaderAllocTraits = std::allocator_traits<HeaderAllocator<Alloc>>;

 public:
  KANON_INLINE ForwardListBase() KANON_NOEXCEPT { reset(); }

  KANON_INLINE ForwardListBase(ForwardListBase &&other) KANON_NOEXCEPT
    : header_(other.header_)
  {
    header_.count = other.header_.count;
    header_.next = other.header_.next;
    header_.prev = (header_.count == 0) ? &header_ : other.header_.prev;
    other.reset();
  }

  // KANON_INLINE ForwardListBase(ForwardListBase const &other) KANON_NOEXCEPT
  //{
  //  header_.count = other.header_.count;
  //  header_.next = other.header_.next;
  //  header_.prev = &header_;
  //}

  KANON_INLINE void swap(ForwardListBase &other) KANON_NOEXCEPT
  {
    std::swap(header_.count, other.header_.count);
    std::swap(header_.next, other.header_.next);
    auto old_header_prev = header_.prev;
    header_.prev = (header_.count == 0) ? &header_ : other.header_.prev;
    other.header_.prev =
        (other.header_.count == 0) ? &other.header_ : old_header_prev;
  }

  KANON_INLINE ~ForwardListBase() KANON_NOEXCEPT {}

  KANON_INLINE void reset() KANON_NOEXCEPT
  {
    header_.prev = &header_;
    header_.next = nullptr;
    header_.count = 0;
  }

 protected:
  Header header_;
};

} // namespace forward_list_detail

/**
 * ForwardList represents a single linked-list
 * The Implementation of STL is so trivial, such that I rewrite, and provide
 * some useage:
 * 1) The interface extract node in list but don't remove it from
 * memory, so we can reuse node.
 * 2) Support push_back() which time complexity is
 * O(1), then we can implementation queue, such as std::queue<ForwardList>(I
 * can't sure it is good, my zstl::queue<Container> support at least)
 */
template <typename T, typename Alloc = std::allocator<T>>
class ForwardList : protected forward_list_detail::ForwardListBase<T, Alloc> {
  using Self = ForwardList;
  using Base = forward_list_detail::ForwardListBase<T, Alloc>;

 public:
  // STL compatible
  using value_type = T;
  using allocator_type = Alloc;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type &;
  using const_reference = value_type const &;
  using pointer = typename std::allocator_traits<Alloc>::pointer;
  using const_pointer = typename std::allocator_traits<Alloc>::const_pointer;
  using iterator = ForwardListIterator<value_type>;
  using const_iterator = ForwardListConstIterator<value_type>;

  using ValueType = T;
  using SizeType = std::size_t;
  using Ref = T &;
  using ConstRef = T const &;
  using Pointer = typename std::allocator_traits<Alloc>::pointer;
  using ConstPointer = typename std::allocator_traits<Alloc>::const_pointer;
  using Iterator = ForwardListIterator<T>;
  using ConstIterator = ForwardListConstIterator<T>;
  using BaseNode = typename Iterator::BaseNode;
  using Node = typename Iterator::Node;
  using ConstNode = typename ConstIterator::Node;
  using NodeAllocator = typename Alloc::template rebind<Node>::other;
  using AllocTraits = std::allocator_traits<NodeAllocator>;

  KANON_INLINE ForwardList() = default;

  KANON_INLINE explicit ForwardList(SizeType n, ValueType const &val = {})
  {
    insert_after(cbefore_begin(), n, val);
  }

  template <typename InputIterator,
            typename = zstl::enable_if_t<
                zstl::is_input_iterator<InputIterator>::value>>
  KANON_INLINE ForwardList(InputIterator first, InputIterator last)
  {
    insert_after(cbefore_begin(), first, last);
  }

  template <typename E>
  KANON_INLINE ForwardList(std::initializer_list<E> il)
    : ForwardList(il.begin(), il.end())
  {
  }

  /**
   * Special member function
   */
  KANON_INLINE ~ForwardList() KANON_NOEXCEPT { clear(); }

  KANON_INLINE ForwardList(ForwardList const &other)
    : Base()
  {
    insert_after(cbefore_begin(), other.cbegin(), other.cend());
  }

  KANON_INLINE ForwardList(ForwardList &&other) KANON_NOEXCEPT
    : Base(std::move(other))
  {
  }

  Self &operator=(Self const &other);

  KANON_INLINE Self &operator=(Self &&other) KANON_NOEXCEPT
  {
    this->swap(other);
    return *this;
  }

  KANON_INLINE allocator_type get_allocator() const KANON_NOEXCEPT
  {
    return allocator_type{};
  }

  void assign(SizeType count, ValueType const &value);
  template <typename InputIterator,
            typename = zstl::enable_if_t<
                zstl::is_input_iterator<InputIterator>::value>>
  void assign(InputIterator first, InputIterator last);
  template <typename U>
  KANON_INLINE void assign(std::initializer_list<U> il)
  {
    assign(il.begin(), il.end());
  }

  KANON_INLINE void resize(SizeType n)
  {
    resize_impl<ValueType>(n, ValueType{});
  }
  void resize(SizeType n, ValueType const &val);

  // Insert after header
  template <typename... Args>
  KANON_INLINE void emplace_front(Args &&...args)
  {
    push_front(create_node(STD_FORWARD(Args, args)...));
  }
  // It is may be better for builtin type that use value-passed parameter.
  // But we can not suppose the move operation for ValueType is cheap
  // Thus, I still write const&/&& or better.
  KANON_INLINE void push_front(ValueType const &val)
  {
    push_front(create_node(val));
  }
  KANON_INLINE void push_front(ValueType &&val)
  {
    push_front(create_node(std::move(val)));
  }

  // Insert before header
  // Not standard required
  template <typename... Args>
  KANON_INLINE void emplace_back(Args &&...args)
  {
    push_back(create_node(STD_FORWARD(Args, args)...));
  }
  KANON_INLINE void push_back(ValueType const &val)
  {
    push_back(create_node(val));
  }
  KANON_INLINE void push_back(ValueType &&val)
  {
    push_back(create_node(std::move(val)));
  }

  // Insert after given position(presented by iterator)
  template <typename... Args>
  KANON_INLINE void emplace_after(ConstIterator pos, Args &&...args)
  {
    insert_after(pos, create_node(STD_FORWARD(Args, args)...));
  }
  KANON_INLINE void insert_after(ConstIterator pos, ValueType const &val)
  {
    insert_after(pos, create_node(val));
  }
  KANON_INLINE void insert_after(ConstIterator pos, ValueType &&val)
  {
    insert_after(pos, create_node(std::move(val)));
  }

  // Based on Node, thus we can reuse extracted node(throught call
  // extract_xxx()) Maybe this is the implementation function of
  // std::forward_list<> But I expose these. Not standard required
  KANON_INLINE void push_front(Iterator node) { push_front(node.extract()); }

  KANON_INLINE void push_back(Iterator node) { push_back(node.extract()); }

  KANON_INLINE void insert_after(ConstIterator pos, Iterator node)
  {
    insert_after(pos, node.extract());
  }

  // Range insert
  void insert_after(ConstIterator pos, SizeType count, ValueType const &val);
  template <typename InputIterator,
            typename = zstl::enable_if_t<
                zstl::is_input_iterator<InputIterator>::value>>
  void insert_after(ConstIterator pos, InputIterator first, InputIterator last);

  // pop/erase/clear
  KANON_INLINE void pop_front();
  KANON_INLINE void pop_front_size(size_t sz);
  KANON_INLINE Iterator erase_after(ConstIterator pos);
  KANON_INLINE Iterator erase_after_size(ConstIterator pos, size_t sz);
  KANON_INLINE Iterator erase_after(ConstIterator first, ConstIterator last);
  KANON_INLINE Iterator erase_after_size(ConstIterator first,
                                         ConstIterator last, size_t sz);
  KANON_INLINE void clear() KANON_NOEXCEPT;

  /**
   * DropCb is a callback that accepts Node* as parameter
   */
  template <typename DropCb>
  KANON_INLINE void clear(DropCb cb) KANON_NOEXCEPT;

  KANON_INLINE void clear_size(size_t sz) KANON_NOEXCEPT;

  // The implematation detail of pop_front() and erase_after()
  // But these don't free the node in fact, just remove it from list
  // The user can reuse the returnd node
  // Not standard required
  KANON_INLINE Iterator extract_front() KANON_NOEXCEPT
  {
    return Iterator(extract_front_node());
  }

  KANON_INLINE Iterator extract_after(ConstIterator pos) KANON_NOEXCEPT
  {
    return Iterator(extract_after_node(pos));
  }

  // accessor
  KANON_INLINE Iterator begin() KANON_NOEXCEPT
  {
    return Iterator(header_.next);
  }
  // Don't write as header_.prev->next since header_.prev may be nullptr
  KANON_INLINE Iterator end() KANON_NOEXCEPT { return Iterator(nullptr); }
  KANON_INLINE ConstIterator begin() const KANON_NOEXCEPT
  {
    return ConstIterator(header_.next);
  }
  KANON_INLINE ConstIterator end() const KANON_NOEXCEPT
  {
    return ConstIterator(nullptr);
  }
  KANON_INLINE ConstIterator cbegin() const KANON_NOEXCEPT { return begin(); }
  KANON_INLINE ConstIterator cend() const KANON_NOEXCEPT { return end(); }
  KANON_INLINE Iterator before_begin() KANON_NOEXCEPT { return &header_; }
  KANON_INLINE ConstIterator before_begin() const KANON_NOEXCEPT
  {
    return &header_;
  }
  KANON_INLINE ConstIterator cbefore_begin() const KANON_NOEXCEPT
  {
    return &header_;
  }

  // Not standard required
  KANON_INLINE Iterator before_end() KANON_NOEXCEPT { return header_.prev; }
  KANON_INLINE ConstIterator before_end() const KANON_NOEXCEPT
  {
    return header_.prev;
  }
  KANON_INLINE ConstIterator cbefore_end() const KANON_NOEXCEPT
  {
    return header_.prev;
  }

  KANON_INLINE Ref front() KANON_NOEXCEPT
  {
    assert(!empty());
    return GET_LINKED_NODE_VALUE(header_.next);
  }
  KANON_INLINE ConstRef front() const KANON_NOEXCEPT
  {
    assert(!empty());
    return GET_LINKED_NODE_VALUE(header_.next);
  }

  // Not standard required
  KANON_INLINE Ref back() KANON_NOEXCEPT
  {
    assert(!empty());
    return GET_LINKED_NODE_VALUE(header_.prev);
  }
  KANON_INLINE ConstRef back() const KANON_NOEXCEPT
  {
    assert(!empty());
    return GET_LINKED_NODE_VALUE(header_.prev);
  }

  // capacity
  KANON_INLINE SizeType max_size() const KANON_NOEXCEPT
  {
    return static_cast<SizeType>(-1);
  }
  KANON_INLINE bool empty() const KANON_NOEXCEPT
  {
    assert(((header_.count == 0) ^ (header_.next == nullptr)) == 0);
    return header_.count == 0;
  }

  // STL don't provide the size() API
  // Not standard required
  KANON_INLINE SizeType size() const KANON_NOEXCEPT { return header_.count; }
  KANON_INLINE void swap(Self &other) KANON_NOEXCEPT;

  // Search the before iterator of the given value.
  // It's useful for calling erase_after() and insert_after().
  // Not standard required
  KANON_INLINE Iterator search_before(ValueType const &val, ConstIterator pos)
  {
    return search_before(
        [&val](ValueType const &value) {
          return value == val;
        },
        pos);
  }

  template <typename UnaryPred>
  Iterator search_before(UnaryPred pred, ConstIterator pos);

  KANON_INLINE Iterator search_before(ValueType const &val)
  {
    return search_before(val, cbefore_begin());
  }
  template <typename UnaryPred>
  KANON_INLINE Iterator search_before(UnaryPred pred)
  {
    return search_before(pred, cbefore_begin());
  }

  // Operations

  /**
   * Let len1 = std::distance(begin(), end()), len2 =
   * std::distance(list.begin(), list.end()) It is O(len1+len2) to
   * std::forward_list<> but this is O(min(len1, len2)) + O(1) = O(min(len1,
   * len2)) It is more efficient.
   */
  KANON_INLINE void merge(Self &list)
  {
    merge(list, [](ValueType const &x, ValueType const &y) {
      return x < y;
    });
  }
  KANON_INLINE void merge(Self &&list) { merge(list); }
  template <typename BinaryPred>
  void merge(Self &list, BinaryPred pred);
  template <typename BinaryPred>
  KANON_INLINE void merge(Self &&list, BinaryPred pred)
  {
    merge(list, std::move(pred));
  }

  /**
   * It is O(n) to std::forward_list<>, but this is O(1) here since header->prev
   */
  void splice_after(ConstIterator pos, Self &list);
  KANON_INLINE void splice_after(ConstIterator pos, Self &&list)
  {
    splice_after(pos, list);
  }
  void splice_after(ConstIterator pos, Self &list, ConstIterator it);
  KANON_INLINE void splice_after(ConstIterator pos, Self &&list,
                                 ConstIterator it)
  {
    splice_after(pos, list, it);
  }
  void splice_after(ConstIterator pos, Self &list, ConstIterator first,
                    ConstIterator last);
  KANON_INLINE void splice_after(ConstIterator pos, Self &&list,
                                 ConstIterator first, ConstIterator last)
  {
    splice_after(pos, list, first, last);
  }

  // In C++20, the return type is modified to size_type(i.e. SizeType here).
  // It indicates the number of removed elements
  KANON_INLINE SizeType remove(ValueType const &val)
  {
    return remove_if([&val](ValueType const &value) {
      return value == val;
    });
  }
  template <typename UnaryPred>
  SizeType remove_if(UnaryPred pred);

  void reverse() KANON_NOEXCEPT;

  // In C++20, the return type is modified to size_type(i.e. SizeType here).
  // It indicates the number of removed elements
  KANON_INLINE SizeType unique()
  {
    return unique([](ValueType const &x, ValueType const &y) {
      return x == y;
    });
  }
  template <typename BinaryPred>
  SizeType unique(BinaryPred pred);

  // O(nlgn) and stable(Don't destroy iterator)
  // Merge sort(but only merge stage, no need to partition)
  KANON_INLINE void sort()
  {
    return sort([](ValueType const &x, ValueType const &y) {
      return x < y;
    });
  }
  template <typename Compare>
  void sort(Compare cmp);

  // O(nlgn)
  // It take the quick sort, but the performance is wrong than sort()
  void sort2();

  KANON_INLINE Node *extract_front_node() KANON_NOEXCEPT;
  KANON_INLINE Node *extract_after_node(ConstIterator pos) KANON_NOEXCEPT;

  KANON_INLINE void push_front(Node *new_node) KANON_NOEXCEPT;
  KANON_INLINE void push_back(Node *new_node) KANON_NOEXCEPT;
  KANON_INLINE void insert_after(ConstIterator pos, Node *new_node);

  /*
   * create_node has two step
   * allocate memory ==> construct object
   * Also drop_node has two step:
   * destroy object ==> free memory(/deallocate)
   */

  // HACK method
  // 实现侵入式链表
  template <typename... Args>
  KANON_INLINE Node *create_node_size(size_t sz, Args &&...args)
  {
    auto node = AllocTraits::allocate(*this, sizeof(Node) + sz);

    node->next = nullptr;
    AllocTraits::construct(*this, &node->val, std::forward<Args>(args)...);

    return node;
  }

  template <typename... Args>
  KANON_INLINE Node *create_node(Args &&...args)
  {
    return create_node_size(0, std::forward<Args>(args)...);
  }

  KANON_INLINE void drop_node(BaseNode *_node) KANON_NOEXCEPT
  {
    auto node = static_cast<Node *>(_node);
    AllocTraits::destroy(*this, node);
    AllocTraits::deallocate(*this, node, sizeof(Node));
  }

  KANON_INLINE void drop_node_size(BaseNode *_node, size_t sz)
  {
    auto node = static_cast<Node *>(_node);
    AllocTraits::destroy(*this, node);
    AllocTraits::deallocate(*this, node, sizeof(Node) + sz);
  }
#ifdef FORWARD_LIST_DEBUG
  // For Debugging
  void print() const KANON_NOEXCEPT;
#endif

 private:
  template <typename U, typename Dummy = zstl::enable_if_t<
                            std::is_default_constructible<U>::value>>
  KANON_INLINE void resize_impl(size_t n)
  {
    resize(n, ValueType{});
  }

  // Expose the header_ in Base class.
  // This is necessary for base class template.
  using Base::header_;
};

} // namespace zstl

#include "forward_list/forward_list_impl.h"

#endif // STL_SUP_FORWARD_LIST_H
