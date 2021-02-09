#include "protoflat_generator.h"

#include <protoflat.h>

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

protoflat::wire_type protoflat_wire_type(const google::protobuf::FieldDescriptor *field_type, bool get_final_type)
{
    using namespace google::protobuf;
    switch (field_type->type())
    {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_BOOL:
    case FieldDescriptor::TYPE_ENUM:
        return get_final_type && field_type->is_packed() ? protoflat::wire_type::length_delimited : protoflat::wire_type::varint;
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_FLOAT:
        return get_final_type && field_type->is_packed() ? protoflat::wire_type::length_delimited : protoflat::wire_type::fixed32;
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_DOUBLE:
        return get_final_type && field_type->is_packed() ? protoflat::wire_type::length_delimited : protoflat::wire_type::fixed64;
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_MESSAGE:
        return protoflat::wire_type::length_delimited;
    default:
        throw std::runtime_error("Unsupported type");
    }
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

std::string protoflat_file_name(const google::protobuf::FileDescriptor *file)
{
    return substitute(file->name(), ".proto", ".protoflat");
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

void generate_oneof(const google::protobuf::OneofDescriptor *oneof_type, Printer &printer)
{
    printer.Println("std::optional<std::variant<");
    printer.Indent();
    for (int i = 0; i < oneof_type->field_count(); ++i)
    {
        printer.Print(protoflat_field_type(oneof_type->field(i)));
        if (i + 1 < oneof_type->field_count())
        {
            printer.Println(",");
        }
        else
        {
            printer.Println();
        }
    }
    printer.Outdent();
    printer.Println(">> " + oneof_type->name() + ";");
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
        if (message_type->field(i)->containing_oneof() == nullptr)
        {
            generate_field(message_type->field(i), printer);
        }
    }

    for (int i = 0; i < message_type->oneof_decl_count(); ++i)
    {
        generate_oneof(message_type->oneof_decl(i), printer);
    }

    printer.Outdent();
    printer.Println("};");
    printer.Println();
}

void generate_type_traits_field_header(const google::protobuf::FieldDescriptor *field_type, Printer &printer)
{
    printer.Println("inline static constexpr field_header " + field_type->name() + "_header{" + std::to_string(field_type->number()) + ", wire_type::" + std::string(protoflat::wire_type_string(protoflat_wire_type(field_type, true))) + "};");
}

void generate_type_traits_field_size(const google::protobuf::FieldDescriptor *field_type, Printer &printer)
{
    auto wire_type = protoflat_wire_type(field_type, false);
    auto field_name = "value." + field_type->name();
    if (wire_type == protoflat::wire_type::length_delimited && field_type->is_repeated())
    {
        printer.Println("for (auto &field : value." + field_type->name() + ")");
        printer.Println("{");
        printer.Indent();

        field_name = "field";
    }

    std::string specialization_type(protoflat::protoflat_specialization_type(wire_type, field_type->is_packed()));
    if (field_type->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
    {
        specialization_type = message_type(field_type->message_type());
    }
    printer.Println("size += type_traits<varint>::size(field_header::encode(" + field_type->name() + "_header));");
    printer.Println("size += type_traits<" + specialization_type + ">::size(" + field_name + ");");

    if (wire_type == protoflat::wire_type::length_delimited && field_type->is_repeated())
    {
        printer.Outdent();
        printer.Println("}");
    }

    printer.Println();
}

void generate_type_traits_field_serialize(const google::protobuf::FieldDescriptor *field_type, Printer &printer)
{
    auto wire_type = protoflat_wire_type(field_type, false);
    if (field_type->is_packed() || field_type->is_repeated() || wire_type == protoflat::wire_type::length_delimited)
    {
        printer.Println("if (!value." + field_type->name() + ".empty())");
    }
    else
    {
        printer.Println("if (value." + field_type->name() + ")");
    }
    printer.Println("{");
    printer.Indent();

    auto field_name = "value." + field_type->name();
    if (wire_type == protoflat::wire_type::length_delimited && field_type->is_repeated())
    {
        printer.Println("for (auto &field : value." + field_type->name() + ")");
        printer.Println("{");
        printer.Indent();

        field_name = "field";
    }

    std::string specialization_type(protoflat::protoflat_specialization_type(wire_type, field_type->is_packed()));
    if (field_type->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
    {
        specialization_type = message_type(field_type->message_type());
    }
    printer.Println("type_traits<varint>::serialize(field_header::encode(" + field_type->name() + "_header), data);");
    printer.Println("type_traits<" + specialization_type + ">::serialize(" + field_name + ", data);");

    if (wire_type == protoflat::wire_type::length_delimited && field_type->is_repeated())
    {
        printer.Outdent();
        printer.Println("}");
    }

    printer.Outdent();
    printer.Println("}");
}

void generate_message_type_traits_size(const google::protobuf::Descriptor *message_type, Printer &printer)
{
    printer.Println("static size_t size(const " + ::message_type(message_type) + " &value)");
    printer.Println("{");
    printer.Indent();
    printer.Println("size_t size = 0;");
    printer.Println();

    for (int i = 0; i < message_type->field_count(); ++i)
    {
        generate_type_traits_field_size(message_type->field(i), printer);
    }

    printer.Println("return size;");
    printer.Outdent();
    printer.Println("}");
}

void generate_message_type_traits_serialize(const google::protobuf::Descriptor *message_type, Printer &printer)
{
    printer.Println("static void serialize(const " + ::message_type(message_type) + " &value, std::string &data)");
    printer.Println("{");
    printer.Indent();

    for (int i = 0; i < message_type->field_count(); ++i)
    {
        generate_type_traits_field_serialize(message_type->field(i), printer);
    }

    printer.Outdent();
    printer.Println("}");
}

void generate_message_type_traits(const google::protobuf::Descriptor *message_type, Printer &printer)
{
    printer.Println("template<>");
    printer.Println("struct type_traits<" + ::message_type(message_type) + ">");
    printer.Println("{");
    printer.Indent();

    for (int i = 0; i < message_type->field_count(); ++i)
    {
        generate_type_traits_field_header(message_type->field(i), printer);
    }

    printer.Println();
    generate_message_type_traits_size(message_type, printer);

    printer.Println();
    generate_message_type_traits_serialize(message_type, printer);

    printer.Outdent();
    printer.Println("};");
    printer.Println();
}

void generate_header(const google::protobuf::FileDescriptor *file, Printer &printer)
{
    printer.Println("#pragma once");
    printer.Println();

    for (int i = 0; i < file->dependency_count(); ++i)
    {
        printer.Println("#include \"" + protoflat_file_name(file->dependency(i)) + ".h\"");
    }
    if (file->dependency_count() > 0)
    {
        printer.Println();
    }

    printer.Println("#include <protoflat.h>");
    printer.Println();
    printer.Println("#include <optional>");
    printer.Println("#include <string>");
    printer.Println("#include <variant>");
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
    printer.Println();

    printer.Println("namespace protoflat");
    printer.Println("{");
    printer.Println();

    for (int i = 0; i < file->message_type_count(); ++i)
    {
        generate_message_type_traits(file->message_type(i), printer);
    }

    printer.Println("}");
}

void generate_source(const google::protobuf::FileDescriptor *file, Printer &printer)
{
    printer.Println("#include \"" + protoflat_file_name(file) + ".h\"");
    printer.Println();
    printer.Println("namespace " + substitute(file->package(), ".", "::"));
    printer.Println("{");
    printer.Println("}");
}

bool ProtoflatGenerator::Generate(const google::protobuf::FileDescriptor *file, const std::string &parameter,
                                  google::protobuf::compiler::GeneratorContext *generator_context, std::string *error) const
{
    auto name = protoflat_file_name(file);

    auto header_stream = generator_context->Open(name + ".h");
    Printer header_printer(header_stream);
    generate_header(file, header_printer);

    auto source_stream = generator_context->Open(name + ".cpp");
    Printer source_printer(source_stream);
    generate_source(file, source_printer);

    return true;
}
