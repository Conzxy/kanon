#include "chunk_list_bench_common.h"

#include "kanon/net/buffer.h"

// Buffer已经被移除了

static void BENCHMARK_BufferReservedArray(benchmark::State& state) {
  g_buf = (char*)malloc(state.range(0));
  Buffer buffer;
  for (auto _ : state) {
    buffer.Append(g_buf, state.range(0));
  }
  ::free(g_buf);
}

static void BENCHMARK_BufferVector(benchmark::State& state) {
  g_buf = (char*)malloc(state.range(0));
  kanon::Buffer buffer;
  for (auto _ : state) {
    buffer.Append(g_buf, state.range(0));
  }
  ::free(g_buf);

}

BENCHMARK_CHUNK_LIST(BufferReservedArray);
BENCHMARK_CHUNK_LIST(BufferVector);
