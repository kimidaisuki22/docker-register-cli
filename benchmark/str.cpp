#include <algorithm>
#include <benchmark/benchmark.h>
#include <cstdlib>
#include <cstring>
#include <map>
#include <unordered_map>
#include <vector>

static void BM_StringCreation(benchmark::State& state) {
  for (auto _ : state)
    std::string empty_string;
}
// Register the function as a benchmark
BENCHMARK(BM_StringCreation);

// Define another benchmark
static void BM_StringCopy(benchmark::State& state) {
  std::string x = "hello";
  for (auto _ : state)
    std::string copy(x);
}
BENCHMARK(BM_StringCopy);


static void BM_Memset(benchmark::State& state){
    std::vector<char> buffer;
    buffer.resize(state.range(0));

    for(auto _ : state){
        memset(buffer.data(), 0, buffer.size());
    }
}

BENCHMARK(BM_Memset)->RangeMultiplier(2)->Range(1, 1 << 16);

int randx(){
    return rand()  ;
}

static void BM_Find(benchmark::State& state){
    std::vector<int> buffer;
    buffer.resize(state.range(0));
    for(auto& v: buffer){
        v = randx();
    }
    int i = randx();

    for(auto _ : state){
        auto iter = std::find(buffer.begin(),buffer.end(),i);
        benchmark::DoNotOptimize(iter);
    }
}

BENCHMARK(BM_Find)->RangeMultiplier(2)->Range(1, 1 << 16)->Complexity();

static void BM_Find_hash_map(benchmark::State& state){
    std::unordered_map<int,int> buffer;
    for(int i=0;i<state.range();i++){
        buffer[randx()] = randx();
    }
    int i = randx();

    for(auto _ : state){
        auto iter = buffer.find(i);
        benchmark::DoNotOptimize(iter);
    }
}

BENCHMARK(BM_Find_hash_map)->RangeMultiplier(2)->Range(1, 1 << 16)->Complexity();

static void BM_Find_map(benchmark::State& state){
    std::map<int,int> buffer;
    for(int i=0;i<state.range();i++){
        buffer[randx()] = randx();
    }
    int i = randx();

    for(auto _ : state){
        auto iter = buffer.find(i);
        benchmark::DoNotOptimize(iter);
    }
}

BENCHMARK(BM_Find_map)->RangeMultiplier(2)->Range(1, 1 << 16)->Complexity();