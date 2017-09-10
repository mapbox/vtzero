#include <benchmark/benchmark.h>
#include <vtzero/version.hpp>

static void bench(benchmark::State& state) // NOLINT google-runtime-references
{
    if (state.thread_index == 0)
    {
    }

    while (state.KeepRunning())
    {
        auto argument = static_cast<std::size_t>(state.range(0));
        benchmark::DoNotOptimize(argument);
    }

    if (state.thread_index == 0)
    {
        // Teardown code here.
    }
}

int main(int argc, char* argv[])
{
    benchmark::RegisterBenchmark("bench", bench) // NOLINT clang-analyzer-cplusplus.NewDeleteLeaks
        ->Threads(2)
        ->Threads(4)
        ->Threads(8)
        ->Arg(100);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    return 0;
}