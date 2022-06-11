#include "chunk_list_bench_common.h"

#define N 1

static void BENCHMARK_BufferNoShrink(benchmark::State& state) {
  BENCHMARK_BufferShrink(state, false, false, N);
}

static void BENCHMARK_BufferShrink(benchmark::State& state) {
  BENCHMARK_BufferShrink(state, false, true, N);
}

static void BENCHMARK_ChunkListNoShrink(benchmark::State& state) {
  BENCHMARK_ChunkListShrink(state, true, false, N);
}

static void BENCHMARK_ChunkListShrink(benchmark::State& state) {
  BENCHMARK_ChunkListShrink(state, false, false, N);
}

BENCHMARK_CHUNK_LIST(ChunkListNoShrink);
BENCHMARK_CHUNK_LIST(BufferNoShrink);

BENCHMARK_CHUNK_LIST(BufferShrink);
BENCHMARK_CHUNK_LIST(ChunkListShrink);

BENCHMARK_MAIN();