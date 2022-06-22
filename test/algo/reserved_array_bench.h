#include "kanon/algo/reserved_array.h"

#include <vector>
#include <string>

#include <benchmark/benchmark.h>

using namespace benchmark;
using namespace kanon::algo;
using namespace std;

template<typename T>
static void BM_Vector(State& state, T const& value) {
  vector<T> arr(4);
  
  const size_t threshold = state.range(0);

  for (auto _ : state) {
    auto size = arr.size();

    arr.resize(arr.size() * 2);

    for (size_t i = size; i < arr.size(); ++i) {
      arr[i] = value;
    }
    
    if (arr.size() > threshold) {
      arr.resize(threshold);
      arr.shrink_to_fit();
    }
  }
}

template<typename T>
static void BM_ReservedArray(State& state, T const& value) {
  ReservedArray<T> arr(4);
  
  const size_t threshold = state.range(0);

  for (auto _ : state) {
    auto size = arr.GetSize();

    arr.Grow(arr.GetSize() * 2);
    
    for (size_t i = size; i < arr.GetSize(); ++i) {
      arr[i] = value;
    }

    if (arr.size() > threshold) {
      arr.Shrink(threshold);
    }
  }
}

#define BENCHMARK_DEF(func, name) \
  BENCHMARK(func)->Name(name)->RangeMultiplier(2)->Range(1024, 1024 * 1024)

