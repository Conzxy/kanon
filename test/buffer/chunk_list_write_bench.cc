#include "chunk_list_bench_common.h"

#define N 8

static void BENCHMARK_ChunkListNoShrinkWithWrite(benchmark::State& state) {
  BENCHMARK_ChunkListShrink(state, true, true, N);
}

static void BENCHMARK_BufferNoShrinkWithWrite(benchmark::State& state) {
  BENCHMARK_BufferShrink(state, true, false, N);
}

static void BENCHMARK_ChunkListShrinkWithWrite(benchmark::State& state) {
  BENCHMARK_ChunkListShrink(state, false, true);
}

static void BENCHMARK_BufferShrinkWithWrite(benchmark::State& state) {
  BENCHMARK_BufferShrink(state, true, true);
}

BENCHMARK_CHUNK_LIST(BufferShrinkWithWrite);
BENCHMARK_CHUNK_LIST(ChunkListShrinkWithWrite);

BENCHMARK_CHUNK_LIST(BufferNoShrinkWithWrite);
BENCHMARK_CHUNK_LIST(ChunkListNoShrinkWithWrite);
BENCHMARK_MAIN();