#include "chunk_list_bench_common.h"

static void BENCHMARK_Buffer(benchmark::State& state) {
  g_buf = (char*)malloc(state.range(0));
  Buffer buffer;
  for (auto _ : state) {
    buffer.Append(g_buf, state.range(0));
  }
  ::free(g_buf);
}

BENCHMARK_CHUNK_LIST(Buffer);