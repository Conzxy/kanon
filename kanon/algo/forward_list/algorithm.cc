#include <assert.h>

#include "kanon/algo/forward_list/algorithm.h"
#include "kanon/util/macro.h"

#define FORWARD_LIST_EMPTY (header->next == nullptr)

namespace zstl {
namespace forward_list_detail {

void push_front(Header *header, BaseLinkedNode *new_node) KANON_NOEXCEPT
{
  // Update header_->prev only when list is empty
  if (FORWARD_LIST_EMPTY) {
    assert(header->prev == header);
    header->prev = new_node;
  }

  new_node->next = header->next;
  header->next = new_node;
}

void push_back(Header *header, BaseLinkedNode *new_node) KANON_NOEXCEPT
{
  if (header->prev != header) {
    header->prev->next = new_node;
  } else {
    assert(header->prev == header);
    assert(FORWARD_LIST_EMPTY);
    header->next = new_node;
  }

  header->prev = new_node;
}

BaseLinkedNode *extract_front(Header *header) KANON_NOEXCEPT
{
  assert(!FORWARD_LIST_EMPTY);

  auto ret = header->next;
  header->next = header->next->next;

  if (FORWARD_LIST_EMPTY) {
    header->prev = header;
  }

  // Reset returned node, to avoid construct loop in some cases
  ret->next = nullptr;

  return ret;
}

void insert_after(Header *header, BaseLinkedNode *pos,
                  BaseLinkedNode *new_node) KANON_NOEXCEPT
{
  // Push to back, update header_->prev
  // If pos == header, and FORWARD_LIST_EMPTY == true
  // pos == heaer->prev also equal to true
  if (pos == header->prev) {
    header->prev = new_node;
  }

  new_node->next = pos->next;
  pos->next = new_node;
}

BaseLinkedNode *extract_after(Header *header,
                              BaseLinkedNode *pos) KANON_NOEXCEPT
{
  KANON_ASSERT(pos != nullptr, "The position argument must not be end()");
  KANON_ASSERT(!FORWARD_LIST_EMPTY, "ForwardList must be not empty");
  KANON_ASSERT(pos->next != nullptr,
               "The next node of the position argument mustn't be nullptr");

  auto ret = pos->next;

  // If pos->next is last element, update it
  if (ret == header->prev) {
    // FIXME Need to update head->prev = nullptr when pos == header
    header->prev = pos;
  }

  pos->next = pos->next->next;

  ret->next = nullptr;
  return ret;
}

BaseLinkedNode *extract_after(Header *header, BaseLinkedNode *first,
                              BaseLinkedNode *last) KANON_NOEXCEPT
{
  // The length of the range must be 1 at least
  assert(first);
  if (first->next != last) {
    // If last is the end iterator, indicates the header_->prev need to update
    // No need to set prev(last)->next to nullptr
    // \see erase_after(first, last)
    if (last == nullptr) {
      header->prev = first;
    }

    auto first_next = first->next;
    first->next = last;

    return first_next;
  }

  return nullptr;
}

void reverse(Header *header) KANON_NOEXCEPT
{
  auto cur = header->next;
  BaseLinkedNode *prev = nullptr;

  auto first_node = header->next;
  header->next = header->prev;
  header->prev = first_node;

  BaseLinkedNode *cur_next = nullptr;
  KANON_UNUSED(cur_next);

  while (cur != nullptr) {
    cur_next = cur->next;
    cur->next = prev;
    prev = cur;
    cur = cur_next;
  }
}

} // namespace forward_list_detail
} // namespace zstl
