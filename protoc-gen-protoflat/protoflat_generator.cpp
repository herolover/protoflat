#include "protoflat_generator.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>

std::string substitute(const std::string &text, std::string_view search, std::string_view replace)
{
    auto result = text;
    size_t pos = 0;
    while ((pos = result.find(search, pos)) != std::string::npos)
    {
        result.replace(pos, search.size(), replace);
        pos += search.size();
    }

    return result;
}

class Printer
{
public:
    Printer(google::protobuf::io::ZeroCopyOutputStream *output)
        : _printer(output, '$')
    {
    }

    void Print(std::string_view text)
    {
        _printer.Print(text.data());
    }

    void Println(std::string_view text = "")
    {
        _printer.Print(text.data());
        _printer.Print("\n");
    }

    void Indent()
    {
        _printer.Indent();
        _printer.Indent();
    }

    void Outdent()
    {
        _printer.Outdent();
        _printer.Outdent();
    }

private:
    google::protobuf::io::Printer _printer;
};

void generate_enum(const google::protobuf::EnumDescriptor *enum_type, Printer &printer)
{
    printer.Println("enum class " + enum_type->name());
    printer.Println("{");
    printer.Indent();

    for (int i = 0; i < enum_type->value_count(); ++i)
    {
        auto enum_value = enum_type->value(i);
        printer.Println(enum_value->name() + " = " + std::to_string(enum_value->number()) + ",");
    }

    printer.Outdent();
    printer.Println("};");
    printer.Println();
}

std::string message_type(const google::protobuf::Descriptor *message_type)
{
    return substitute(message_type->full_name(), ".", "::");
}

std::string protoflat_field_type(const google::protobuf::FieldDescriptor *field_type)
{
    using namespace google::protobuf;
    switch (field_type->cpp_type())
    {
    case FieldDescriptor::CPPTYPE_INT32:
        return "int32_t";
    case FieldDescriptor::CPPTYPE_INT64:
        return "int64_t";
    case FieldDescriptor::CPPTYPE_UINT32:
        return "uint32_t";
    case FieldDescriptor::CPPTYPE_UINT64:
        return "uint64_t";
    case FieldDescriptor::CPPTYPE_DOUBLE:
        return "double";
    case FieldDescriptor::CPPTYPE_FLOAT:
        return "float";
    case FieldDescriptor::CPPTYPE_BOOL:
        return "bool";
    case FieldDescriptor::CPPTYPE_STRING:
        return "std::string";
    case FieldDescriptor::CPPTYPE_MESSAGE:
        return message_type(field_type->message_type());
    default:
        return "unknown";
    }
}

void generate_field(const google::protobuf::FieldDescriptor *field_type, Printer &printer)
{
    if (field_type->is_repeated())
    {
        printer.Print("std::vector<");
    }
    printer.Print(protoflat_field_type(field_type));
    if (field_type->is_repeated())
    {
        printer.Print(">");
    }

    printer.Println(" " + field_type->name() + ";");
}

void generate_message(const google::protobuf::Descriptor *message_type, Printer &printer)
{
    printer.Println("struct " + message_type->name());
    printer.Println("{");
    printer.Indent();

    for (int i = 0; i < message_type->enum_type_count(); ++i)
    {
        generate_enum(message_type->enum_type(i), printer);
    }

    for (int i = 0; i < message_type->nested_type_count(); ++i)
    {
        generate_message(message_type->nested_type(i), printer);
    }

    for (int i = 0; i < message_type->field_count(); ++i)
    {
        generate_field(message_type->field(i), printer);
    }

    printer.Outdent();
    printer.Println("};");
    printer.Println();
}

void generate_header(const std::string &/*name*/, const google::protobuf::FileDescriptor *file, Printer &printer)
{
    printer.Println("#pragma once");
    printer.Println();

    printer.Println("#include <string>");
    printer.Println("#include <vector>");
    printer.Println();

    printer.Println("namespace " + substitute(file->package(), ".", "::"));
    printer.Println("{");
    printer.Println();

    for (int i = 0; i < file->enum_type_count(); ++i)
    {
        generate_enum(file->enum_type(i), printer);
    }

    for (int i = 0; i < file->message_type_count(); ++i)
    {
        generate_message(file->message_type(i), printer);
    }

    printer.Println("}");
}

void generate_source(const std::string &name, const google::protobuf::FileDescriptor *file, Printer &printer)
{
    printer.Println("#include \"" + name + ".h\"");
    printer.Println();
    printer.Println("namespace " + substitute(file->package(), ".", "::"));
    printer.Println("{");
    printer.Println("}");
}

bool ProtoflatGenerator::Generate(const google::protobuf::FileDescriptor *file, const std::string &/*parameter*/,
                                  google::protobuf::compiler::GeneratorContext *generator_context, std::string */*error*/) const
{
    auto name = substitute(file->name(), ".proto", ".protoflat");

    auto header_stream = generator_context->Open(name + ".h");
    Printer header_printer(header_stream);
    generate_header(name, file, header_printer);

    auto source_stream = generator_context->Open(name + ".cpp");
    Printer source_printer(source_stream);
    generate_source(name, file, source_printer);

    return true;
}
