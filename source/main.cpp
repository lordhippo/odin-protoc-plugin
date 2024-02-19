#include "odin-code-generator.h"

int main(int argc, char *argv[])
{
    OdinCodeGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
