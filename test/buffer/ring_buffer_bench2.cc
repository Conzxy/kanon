#include "kanon/buffer/ring_buffer.h"

#include <random>

#include <gtest/gtest.h>
#include <benchmark/benchmark.h>

#include "kanon/net/buffer.h"

char g_buf[65535];

void BENCHMARK_buffer(benchmark::State& state) {
  kanon::Buffer buf;
  buf.ReserveWriteSpace(sizeof g_buf);

  std::default_random_engine dre;
  std::uniform_int_distribution<int> uid(1, 65535);
 
  for (auto _ : state) {
    auto len = uid(dre);
    if (buf.GetWritableSize() < 65535) {
      buf.AdvanceAll();
    }
    buf.Append(g_buf, len);
  }
}

void BENCHMARK_ring_buffer(benchmark::State& state) {
  kanon::RingBuffer<char> buf(sizeof g_buf);

  std::default_random_engine dre;
  std::uniform_int_distribution<int> uid(1, 65535);
 
  for (auto _ : state) {
    auto len = uid(dre);
    buf.Append(g_buf, len);
  }
}

BENCHMARK(BENCHMARK_buffer);
BENCHMARK(BENCHMARK_ring_buffer);

BENCHMARK_MAIN();
