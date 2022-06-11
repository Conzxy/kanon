#ifndef KANON_TEST_BUFFER_CHUNK_LIST_BENCH_COMMON_H
#define KANON_TEST_BUFFER_CHUNK_LIST_BENCH_COMMON_H

#include "kanon/net/buffer.h"
#include "kanon/buffer/chunk_list.h"

#include <random>
#include <iostream>
#include <benchmark/benchmark.h>
#include <gtest/gtest.h>

using kanon::ChunkList;
using kanon::Buffer;

#define BENCHMARK_CHUNK_LIST(name) \
  BENCHMARK(BENCHMARK_##name)->Name(#name)->RangeMultiplier(2)->Range(4096, 1024 * 1024)

static char* g_buf;
static int null_fd = ::open("/dev/null", O_WRONLY);

inline void BENCHMARK_ChunkListShrink(benchmark::State& state, bool need_free, bool need_write,  int n=1) {
  ChunkList buffer;

  g_buf = (char*)::malloc(state.range(0));
  auto buffer_size = state.range(0);
  auto count = buffer_size / n;
  std::default_random_engine dre;
  std::uniform_int_distribution<size_t> uid(1, count);

  for (auto _ : state) {
    // EXPECT_EQ(buffer.GetChunkSize(), 0);
    // EXPECT_TRUE(!buffer.HasReadable());

    for (int i = 0; i < n; ++i) {
      buffer.Append(g_buf, uid(dre));
    }

    // EXPECT_EQ(buffer.GetReadableSize(), buffer_size); 
    // EXPECT_EQ(buffer.GetChunkSize(), buffer_size >> 12);
    // EXPECT_EQ(buffer.GetFreeChunkSize(), 0);

    if (need_write) {
      buffer.WriteFd(null_fd);
    }

    buffer.AdvanceRead(buffer.GetReadableSize());

    if (!need_free) { 
      buffer.Shrink(0);
    }

    // EXPECT_EQ(buffer.GetFreeChunkSize(), need_free ? state.range(0) >> 12 : 0);
  }
  ::free(g_buf);
}

inline void BENCHMARK_BufferShrink(benchmark::State& state, bool need_write, bool need_shrink, int n=1) {
  Buffer buffer;

  auto buffer_size = state.range(0);
  auto count = buffer_size / n;
  std::default_random_engine dre;
  std::uniform_int_distribution<size_t> uid(1, count);

  g_buf = (char*)::malloc(state.range(0));
  for (auto _ : state) {
    for (int i = 0; i < n; ++i) {
      buffer.Append(g_buf, uid(dre));
    }

    if (need_write) {
      ::write(null_fd, buffer.GetReadBegin(), buffer.GetReadableSize());
    }
    buffer.AdvanceRead(buffer.GetReadableSize());

    if (need_shrink) buffer.Shrink(0);
  }
  ::free(g_buf);
}

// 暂时结论：
// ChunkList相较于Buffer缺点：
// 1）多个节点需要多次分配（可以通过free_list缓解）
// 2）结点不是连续的，局部性不佳
// 3）需要使用writev()，相比write()性能更低
// 优点：
// 1）空间利用率高，不需要主动shrink，且shrink本身不需要重分配和memcpy
// 2）resize()不需要重分配和memcpy

#endif // 