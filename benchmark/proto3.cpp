#include "../tests/test2.pb.h"

#include <benchmark/benchmark.h>

static void BM_Proto3SerializeToStringWithoutAlloc(benchmark::State &state)
{
    test2::Message message;
    message.set_id(1000);
    message.set_text("Hello!");
    message.add_data(1'000'000);
    message.add_data(-5000);
    message.add_data(-1);
    message.add_messages("Hello!");
    message.add_messages("World!");

    std::string data;
    data.reserve(1000);
    for (auto _ : state)
    {
        message.SerializeToString(&data);
        data.clear();
    }
}

static void BM_Proto3SerializeAsString(benchmark::State &state)
{
    test2::Message message;
    message.set_id(1000);
    message.set_text("Hello!");
    message.add_data(1'000'000);
    message.add_data(-5000);
    message.add_data(-1);
    message.add_messages("Hello!");
    message.add_messages("World!");

    for (auto _ : state)
    {
        message.SerializeAsString();
    }
}

BENCHMARK(BM_Proto3SerializeToStringWithoutAlloc);
BENCHMARK(BM_Proto3SerializeAsString);
