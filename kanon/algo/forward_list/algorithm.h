#ifndef FORWARD_LIST_ALGORITHM_H
#define FORWARD_LIST_ALGORITHM_H

#include "kanon/util/macro.h"
#include "node_def.h"

// In C++17 you can write: namespace zstl::forward_list_detail {
namespace zstl {
namespace forward_list_detail {

/**
 * Because only the value of node is related with template parameter,
 * we can use @c BaseLinkedNode to split the detail of algorithm to avoid every
 * class template instantiation with implemetation itself.
 *
 * The push/insert_xxx() and extract_xxx() no need to consider the value of node
 */
KANON_CORE_API void push_front(Header *header,
                               BaseLinkedNode *new_node) KANON_NOEXCEPT;
KANON_CORE_API void push_back(Header *header,
                              BaseLinkedNode *new_node) KANON_NOEXCEPT;
KANON_CORE_API void insert_after(Header *header, BaseLinkedNode *pos,
                                 BaseLinkedNode *new_node) KANON_NOEXCEPT;

KANON_CORE_API BaseLinkedNode *extract_front(Header *header) KANON_NOEXCEPT;
KANON_CORE_API BaseLinkedNode *
extract_after(Header *header, BaseLinkedNode *pos) KANON_NOEXCEPT;
KANON_CORE_API BaseLinkedNode *
extract_after(Header *header, BaseLinkedNode *first,
              BaseLinkedNode *lsat) KANON_NOEXCEPT;
KANON_CORE_API void reverse(Header *header) KANON_NOEXCEPT;

} // namespace forward_list_detail
} // namespace zstl

#endif // FORWARD_LIST_ALGORITHM_H
