#include "kanon/algo/RingBuffer.h"

using namespace kanon;

int main() {
  RingBuffer<char> ring_buffer(12);

  assert(ring_buffer.readable() == 0);
  assert(ring_buffer.writeable() == ring_buffer.maxSize());

  char const* data1 = "1234567890";
  ring_buffer.append(data1, 10);
  assert(! ::strncmp(ring_buffer.peek(), data1, 10));

  char buf[4096];

  __builtin_memmove(buf, ring_buffer.peek(), 5);
  ring_buffer.advance(5);
  assert(! ::strncmp(buf, data1, 5));

  __builtin_memmove(buf, ring_buffer.peek(), 5);
  ring_buffer.advance(5);
  assert(! ::strncmp(buf, data1+5, 5));
}