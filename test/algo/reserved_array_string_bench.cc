#include "reserved_array_bench.h"

static void BM_VectorString(State& state) {
  BM_Vector(state, string("A"));
}


static void BM_ReservedArrayString(State& state) {
  BM_ReservedArray(state, string("A"));
}

BENCHMARK_DEF(BM_VectorString, "std::vector<std::string>");
BENCHMARK_DEF(BM_ReservedArrayString, "kanon::algo::ReservedArray<std::string>");
