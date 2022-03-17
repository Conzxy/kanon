#include "kanon/algo/ring_buffer.h"

using namespace kanon;

int main() {
  RingBuffer<char> ring_buffer(12);

  assert(ring_buffer.readable() == 0);
  assert(ring_buffer.writeable() == ring_buffer.max_size());

  char const* data1 = "1234567890";
  ring_buffer.Append(data1, 10);
  assert(! ::strncmp(ring_buffer.GetReadBegin(), data1, 10));

  char buf[4096];

  __builtin_memmove(buf, ring_buffer.GetReadBegin(), 5);
  ring_buffer.AdvanceRead(5);
  assert(! ::strncmp(buf, data1, 5));

  __builtin_memmove(buf, ring_buffer.GetReadBegin(), 5);
  ring_buffer.AdvanceRead(5);
  assert(! ::strncmp(buf, data1+5, 5));
}