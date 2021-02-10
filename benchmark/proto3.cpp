#include "../tests/test.pb.h"

#include <benchmark/benchmark.h>

static void BM_Proto3SerializeToStringWithoutAlloc(benchmark::State &state)
{
    test::Message message;
    message.add_data();

    std::string buffer;
    buffer.reserve(1000);
    for (auto _ : state)
    {
        message.SerializeToString(&buffer);
        buffer.clear();
    }
}

static void BM_Proto3SerializeAsString(benchmark::State &state)
{
    test::Message message;
    message.add_data();

    for (auto _ : state)
    {
        message.SerializeAsString();
    }
}

BENCHMARK(BM_Proto3SerializeToStringWithoutAlloc);
BENCHMARK(BM_Proto3SerializeAsString);
