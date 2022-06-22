#include "reserved_array_bench.h"

static void BM_VectorInt(State& state) {
  BM_Vector(state, 1);
}

static void BM_ReservedArrayInt(State& state) {
  BM_ReservedArray(state, 1);
}

BENCHMARK_DEF(BM_VectorInt, "std::vector<int>");
BENCHMARK_DEF(BM_ReservedArrayInt, "kanon::algo::ReservedArray<int>");
