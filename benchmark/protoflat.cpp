#include "../tests/test2.protoflat.h"

#include <benchmark/benchmark.h>

static void BM_ProtoflatSerializeToStringWithoutAlloc(benchmark::State &state)
{
    test2::Message message{1000, "Hello!", {1'000'000, -5000, -1}, {"Hello!", "World!"}};

    std::string data;
    data.reserve(1000);
    for (auto _ : state)
    {
        protoflat::serialize_to_string(message, data);
        data.clear();
    }
}

static void BM_ProtoflatSerializeAsString(benchmark::State &state)
{
    test2::Message message{1000, "Hello!", {1'000'000, -5000, -1}, {"Hello!", "World!"}};

    for (auto _ : state)
    {
        protoflat::serialize(message);
    }
}

BENCHMARK(BM_ProtoflatSerializeToStringWithoutAlloc);
BENCHMARK(BM_ProtoflatSerializeAsString);
