#include "../tests/test.protoflat.h"

#include <benchmark/benchmark.h>

static void BM_ProtoflatSerializeToStringWithoutAlloc(benchmark::State &state)
{
    test::Message message;
    message.data.emplace_back();

    std::string buffer;
    buffer.reserve(1000);
    for (auto _ : state)
    {
        protoflat::serialize_to_string(message, buffer);
        buffer.clear();
    }
}

static void BM_ProtoflatSerializeAsString(benchmark::State &state)
{
    test::Message message;
    message.data.emplace_back();

    for (auto _ : state)
    {
        protoflat::serialize(message);
    }
}

BENCHMARK(BM_ProtoflatSerializeToStringWithoutAlloc);
BENCHMARK(BM_ProtoflatSerializeAsString);
