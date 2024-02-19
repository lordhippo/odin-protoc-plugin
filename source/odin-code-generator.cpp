#include "odin-code-generator.h"

bool OdinCodeGenerator::Generate(
    const google::protobuf::FileDescriptor *const file,
    const std::string &parameter,
    google::protobuf::compiler::GeneratorContext *const generator_context,
    std::string *const error) const
{
    auto* output = generator_context->Open(file->name() + ".odin");

    google::protobuf::io::Printer printer(output, '$');

    printer.Print("package protos\n\n");

    for (int i = 0; i < file->message_type_count(); ++i)
    {
        GenerateMessage(file->message_type(i), &printer);
    }
    return true;
}

void OdinCodeGenerator::GenerateMessage(
    const google::protobuf::Descriptor *const message,
    google::protobuf::io::Printer *const      printer) const
{
    printer->Print("$name$ :: struct {\n", "name", message->name());
    printer->Indent();

    for (int i = 0; i < message->field_count(); i++)
    {
        auto *field = message->field(i);
        printer->Print("$name$ : $type$\n", 
            "name", field->name(), 
            "type", field->type_name());
    }

    // TODO: add fields

    printer->Outdent();
    printer->Print("}\n");

    for (int i = 0; i < message->nested_type_count(); ++i)
    {
        GenerateMessage(message->nested_type(i), printer);
    }
}
