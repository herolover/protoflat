#include "protoflat_generator.h"

#include <google/protobuf/compiler/plugin.h>

int main(int argc, char *argv[])
{
    ProtoflatGenerator protoflat_generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &protoflat_generator);
}
