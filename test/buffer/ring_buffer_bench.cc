#include <benchmark/benchmark.h>

#include "kanon/buffer/ring_buffer.h"
#include "kanon/net/buffer.h"

char buf[65536];

using namespace kanon;

static void BM_RingBufferAppend(benchmark::State& state)
{
  RingBuffer buffer(65535);
  for (auto _ : state) {
    buffer.Append(buf, (sizeof buf - 1) / 2);
    buffer.Append(buf, (sizeof buf - 1));
  }
}

BENCHMARK(BM_RingBufferAppend);

static void BM_BufferAppend(benchmark::State& state)
{
  Buffer buffer;
  buffer.ReserveWriteSpace(65536);
  for (auto _ : state) {
    buffer.Append(buf, (sizeof buf - 1) / 2);
    buffer.AdvanceAll();
    buffer.Append(buf, (sizeof buf - 1));
  }
}

BENCHMARK(BM_BufferAppend);

BENCHMARK_MAIN();