#include "chunk_list_bench_common.h"

static void BENCHMARK_ChunkList(benchmark::State& state, int n=1) {
  ChunkList buffer;
  auto buffer_size = state.range(0);
  auto count = buffer_size / n;

  std::default_random_engine dre;
  std::uniform_int_distribution<size_t> uid(1, count);
  g_buf = (char*)malloc(state.range(0));

  for (auto _ : state) {
    for (int i = 0; i < n; ++i) {
      buffer.Append(g_buf, uid(dre));
    }
  }
  ::free(g_buf);
}

static void BENCHMARK_Buffer(benchmark::State& state, int n=1) {
  g_buf = (char*)malloc(state.range(0));
  Buffer buffer;
  auto buffer_size = state.range(0);
  auto count = buffer_size / n;

  std::default_random_engine dre;
  std::uniform_int_distribution<size_t> uid(1, count);

  for (auto _ : state) {
    for (int i = 0; i < n; ++i) {
      buffer.Append(g_buf, uid(dre));
    }
  }
  ::free(g_buf);
}

static void BENCHMARK_Vector(benchmark::State& state, int n=1) {
  g_buf = (char*)malloc(state.range(0));

  std::vector<char> buffer;
  auto buffer_size = state.range(0);
  auto count = buffer_size / n;

  std::default_random_engine dre;
  std::uniform_int_distribution<size_t> uid(1, count);

  for (auto _ : state) {
    for (int i = 0; i < n; ++i) {
      buffer.insert(buffer.end(), g_buf, g_buf+uid(dre));
    }
  }
  ::free(g_buf);

}

#define N 1

static void BENCHMARK_Buffer(benchmark::State& state) {
  BENCHMARK_Buffer(state, N);
}

static void BENCHMARK_ChunkList(benchmark::State& state) {
  BENCHMARK_ChunkList(state, N);
}

static void BENCHMARK_Vector(benchmark::State& state) {
  BENCHMARK_Vector(state, N);
}

BENCHMARK_CHUNK_LIST(Buffer);
BENCHMARK_CHUNK_LIST(ChunkList);
BENCHMARK_CHUNK_LIST(Vector);
BENCHMARK_MAIN();