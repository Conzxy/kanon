#ifndef STL_SUP_FORWARD_LIST_NODE_DEF_H
#define STL_SUP_FORWARD_LIST_NODE_DEF_H

#include <stddef.h>
#include <utility>
#include <stdio.h>

// Hide the implementation detail for users
namespace zstl {
namespace forward_list_detail {

/**
 * Throught BaseLinkedNode to split the detail of algorithm
 * from class template(ForwardList<>)
 * @note In C++, "typedef struct XXX { } XXX;" is not necessary in fact.
 * @see src/forward_list/algorithm.cc
 */
typedef struct BaseLinkedNode {
  struct BaseLinkedNode* next = nullptr;
} BaseLinkedNode;

/**
 * Represents the plain node
 * which has value(related template parameter)
 */
template<typename T>
struct LinkedNode : BaseLinkedNode {
  T val;
};

/**
 * Represents the header sentinel.
 * Different from the plain node(LinkedNode<>), header doesn't maintains value domain,
 * it just maintains previous/next and count of elements.
 * @warning This is not STL-required, just useful for me.
 */
struct Header : BaseLinkedNode {
  BaseLinkedNode* prev = nullptr;
  size_t count = 0;
};

// Useful macro for ForwardList<>
#define GET_LINKED_NODE_VALUE(node) \
  static_cast<Node*>(node)->val

#define SET_LINKED_NODE_VALUE(node, val) \
  static_cast<Node*>(node)->val = val

} // namespace forward_list_detail
} // namespace zstl

#endif // STL_SUP_FORWARD_LIST_NODE_DEF_H