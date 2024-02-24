#include "odin-code-generator.h"

using namespace google::protobuf;

struct Context
{
	google::protobuf::io::Printer printer;
	std::string *error;
	std::string_view proto_package;
};

// replaces '.' with '_' in type names.
// idx.e. FirstType.SecondType -> FirstType_SecondType
static std::string ConvertFullTypeName(const std::string_view full_name, const std::string_view package_name)
{
	const size_t offset = package_name.empty() ? 0 : package_name.size() + 1;
	std::string result{full_name.substr(offset)};
	std::replace(result.begin(), result.end(), '.', '_');
	return result;
}

enum class OdinBuiltinType
{
	t_none = 0,
	t_i32 = 1,		   // TYPE_INT32, TYPE_SINT32, TYPE_SFIXED32
	t_i64 = 2,		   // TYPE_INT64, TYPE_SINT64, TYPE_SFIXED64
	t_u32 = 3,		   // TYPE_UINT32, TYPE_FIXED32
	t_u64 = 4,		   // TYPE_UINT64, TYPE_FIXED64
	t_f64 = 5,		   // TYPE_DOUBLE
	t_f32 = 6,		   // TYPE_FLOAT
	t_bool = 7,		   // TYPE_BOOL
	t_enum = 8,		   // TYPE_ENUM
	t_string = 9,	   // TYPE_STRING
	t_byte_slice = 10, // TYPE_BYTES
	t_message = 11,	   // TYPE_MESSAGE, TYPE_GROUP
};

static OdinBuiltinType GetOdinBuiltinType(const FieldDescriptor::Type type)
{
	switch (type)
	{
	case FieldDescriptor::TYPE_DOUBLE:
		return OdinBuiltinType::t_f64;
	case FieldDescriptor::TYPE_FLOAT:
		return OdinBuiltinType::t_f32;
	case FieldDescriptor::TYPE_INT64:
		return OdinBuiltinType::t_i64;
	case FieldDescriptor::TYPE_UINT64:
		return OdinBuiltinType::t_u64;
	case FieldDescriptor::TYPE_INT32:
		return OdinBuiltinType::t_i32;
	case FieldDescriptor::TYPE_FIXED64:
		return OdinBuiltinType::t_u64;
	case FieldDescriptor::TYPE_FIXED32:
		return OdinBuiltinType::t_u32;
	case FieldDescriptor::TYPE_BOOL:
		return OdinBuiltinType::t_bool;
	case FieldDescriptor::TYPE_STRING:
		return OdinBuiltinType::t_string;
	case FieldDescriptor::TYPE_GROUP:
		return OdinBuiltinType::t_message;
	case FieldDescriptor::TYPE_MESSAGE:
		return OdinBuiltinType::t_message;
	case FieldDescriptor::TYPE_BYTES:
		return OdinBuiltinType::t_byte_slice;
	case FieldDescriptor::TYPE_UINT32:
		return OdinBuiltinType::t_u32;
	case FieldDescriptor::TYPE_ENUM:
		return OdinBuiltinType::t_enum;
	case FieldDescriptor::TYPE_SFIXED32:
		return OdinBuiltinType::t_i32;
	case FieldDescriptor::TYPE_SFIXED64:
		return OdinBuiltinType::t_i64;
	case FieldDescriptor::TYPE_SINT32:
		return OdinBuiltinType::t_i32;
	case FieldDescriptor::TYPE_SINT64:
		return OdinBuiltinType::t_i64;
	default:
		return OdinBuiltinType::t_none;
	}
}

static std::string_view GetOdinBuiltinTypeName(const OdinBuiltinType type)
{
	switch (type)
	{
	case OdinBuiltinType::t_none:
		return "";
	case OdinBuiltinType::t_i32:
		return "i32";
	case OdinBuiltinType::t_i64:
		return "i64";
	case OdinBuiltinType::t_u32:
		return "u32";
	case OdinBuiltinType::t_u64:
		return "u64";
	case OdinBuiltinType::t_f64:
		return "f64";
	case OdinBuiltinType::t_f32:
		return "f32";
	case OdinBuiltinType::t_bool:
		return "bool";
	case OdinBuiltinType::t_enum:
		return "";
	case OdinBuiltinType::t_string:
		return "string";
	case OdinBuiltinType::t_byte_slice:
		return "[]u8";
	case OdinBuiltinType::t_message:
		return "";
	default:
		return "";
	}
}

static std::string GetOdinFieldTypeName(const FieldDescriptor &field_desc, const std::string_view package_name)
{
	std::string type_name{};

	if (const Descriptor *const message_desc = field_desc.message_type(); message_desc)
	{
		type_name = ConvertFullTypeName(message_desc->full_name(), package_name);
	}
	else if (const EnumDescriptor *const enum_desc = field_desc.enum_type(); enum_desc)
	{
		type_name = ConvertFullTypeName(enum_desc->full_name(), package_name);
	}
	else
	{
		const OdinBuiltinType odin_type = GetOdinBuiltinType(field_desc.type());
		type_name = GetOdinBuiltinTypeName(odin_type);
	}

	if (field_desc.is_repeated())
	{
		type_name = std::format("[]({})", type_name);
	}

	return type_name;
}

static std::string GetFieldTags(const FieldDescriptor &field_desc)
{
	std::string result{};

	if (field_desc.is_packed())
	{
		result += " packed";
	}

	return result;
}

static bool PrintField(const FieldDescriptor &field_desc, Context *const context)
{
	// TODO: handle maps
	// TODO: handle default values

	const std::map<std::string, std::string> vars{
		{"name", field_desc.name()},
		{"odin_type", GetOdinFieldTypeName(field_desc, context->proto_package)},
		{"id", std::to_string(field_desc.number())},
		{"proto_type", std::to_string((int) field_desc.type())},
		{"tags", GetFieldTags(field_desc)},
	};

	context->printer.Print(vars, "$name$ : $odin_type$ `id:\"$id$\" type:\"$proto_type$\"$tags$`,\n");

	return true;
}

static bool PrintOneof(const OneofDescriptor &oneof_desc, Context *const context)
{
	std::map<std::string, std::string> vars{
		{"name", oneof_desc.name()},
	};

	context->printer.Print(vars, "$name$: struct #raw_union {\n");
	context->printer.Indent();

	for (int idx = 0; idx < oneof_desc.field_count(); ++idx)
	{
		if (!PrintField(*oneof_desc.field(idx), context))
		{
			return false;
		}
	}

	context->printer.Outdent();
	context->printer.Print("}\n\n");

	return true;
}

static bool PrintEnum(const EnumDescriptor &enum_desc, Context *const context)
{
	std::map<std::string, std::string> vars{
		{"name", ConvertFullTypeName(enum_desc.full_name(), context->proto_package)},
	};

	context->printer.Print(vars, "$name$ :: enum {\n");
	context->printer.Indent();

	vars.clear();

	for (int idx = 0; idx < enum_desc.value_count(); ++idx)
	{
		const EnumValueDescriptor &value_desc = *enum_desc.value(idx);

		vars["name"] = value_desc.name();
		vars["value"] = std::to_string(value_desc.number());

		context->printer.Print(vars, "$name$ = $value$,\n");
	}

	context->printer.Outdent();
	context->printer.Print("}\n\n");

	return true;
}

static bool PrintMessage(const Descriptor &message_desc, Context *const context)
{
	const std::map<std::string, std::string> vars{
		{"name", ConvertFullTypeName(message_desc.full_name(), context->proto_package)},
	};

	context->printer.Print(vars, "$name$ :: struct {\n");
	context->printer.Indent();

	for (int idx = 0; idx < message_desc.field_count(); ++idx)
	{
		if (!PrintField(*message_desc.field(idx), context))
		{
			return false;
		}
	}

	context->printer.Outdent();
	context->printer.Print("}\n\n");

	for (int idx = 0; idx < message_desc.nested_type_count(); ++idx)
	{
		if (!PrintMessage(*message_desc.nested_type(idx), context))
		{
			return false;
		}
	}

	for (int idx = 0; idx < message_desc.enum_type_count(); ++idx)
	{
		if (!PrintEnum(*message_desc.enum_type(idx), context))
		{
			return false;
		}
	}

	for (int idx = 0; idx < message_desc.oneof_decl_count(); ++idx)
	{
		if (!PrintOneof(*message_desc.oneof_decl(idx), context))
		{
			return false;
		}
	}

	return true;
}

static bool PrintFile(const FileDescriptor &file_desc, Context *const context)
{
	std::string package_name = "proto";
	if (!file_desc.package().empty())
	{
		package_name += '_';
		package_name += ConvertFullTypeName(file_desc.package(), "");
	}

	const std::map<std::string, std::string> vars{
		{"package", package_name},
	};

	context->printer.Print(vars, "package $package$\n\n");

	// TODO: handle dependencies, i.e. file_desc.dependency and file_desc.public_dependency

	for (int idx = 0; idx < file_desc.message_type_count(); ++idx)
	{
		if (!PrintMessage(*file_desc.message_type(idx), context))
		{
			return false;
		}
	}

	for (int idx = 0; idx < file_desc.enum_type_count(); ++idx)
	{
		if (!PrintEnum(*file_desc.enum_type(idx), context))
		{
			return false;
		}
	}

	return true;
}

bool OdinCodeGenerator::Generate(const FileDescriptor *const file, const std::string &parameter,
								 compiler::GeneratorContext *const generator_context, std::string *const error) const
{
	const std::string output_filename = compiler::StripProto(file->name()) + ".pb.odin";
	auto *output = generator_context->Open(output_filename);

	Context context{
		.printer = {output, '$'},
		.error = error,
		.proto_package = file->package(),
	};

	compiler::Version compiler_version;
	generator_context->GetCompilerVersion(&compiler_version);

	const std::map<std::string, std::string> vars{
		{"compiler_version_major", std::to_string(compiler_version.major())},
		{"compiler_version_minor", std::to_string(compiler_version.minor())},
		{"compiler_version_patch", std::to_string(compiler_version.patch())},
	};

	context.printer.Print(
		vars, "// Auto-generated by odin-protoc-plugin (https://github.com/lordhippo/odin-protoc-plugin)\n");
	context.printer.Print(
		vars, "// protoc version: $compiler_version_major$.$compiler_version_minor$.$compiler_version_patch$\n");
	context.printer.Print(
		vars, "// Use with the runtime odin-protobuf library (https://github.com/lordhippo/odin-protobuf)\n");

	return PrintFile(*file, &context);
}
