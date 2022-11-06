#include "../bpc_code_gen.h"

static const char* csharp_basic_type[] = {
	"float", "double", "sbyte", "short", "int", "long", "byte", "ushort", "uint", "ulong", "string"
};
static const char* csharp_formal_basic_type[] = {
	"Single", "Double", "SByte", "Int16", "Int32", "Int64", "Byte", "UInt16", "UInt32", "UInt64", "String"
};
// Ref: https://learn.microsoft.com/en-us/dotnet/api/system.runtime.interopservices.unmanagedtype?view=net-6.0
static const char* csharp_unmanaged_type[] = {
	"R4", "R8", "I1", "I2", "I4", "I8", "U1", "U2", "U4", "U8", "LPUTF8Str"	// the unmanaged type of string still is invalid. LPUTF8Str just is a placeholder.
};

static void write_natural_struct(FILE* fs, BPCSMTV_STRUCT_MODIFIER modifier, GSList* variables, BPCGEN_INDENT_TYPE indent) {
	// check requirement
	g_assert(!modifier.is_narrow);

	// generate internal struct
	// Ref: https://learn.microsoft.com/en-us/dotnet/framework/interop/marshalling-classes-structures-and-unions
	BPCGEN_INDENT_INIT_REF(fs, indent);
	GSList* cursor;
	uint32_t offsets = UINT32_C(0);

	BPCGEN_INDENT_PRINT;
	fprintf(fs, "[StructLayout(LayoutKind.Explicit, Size = %" PRIu32 ")]", modifier.struct_size);
	BPCGEN_INDENT_PRINT;
	fputs("public struct _NaturalStruct {", fs);
	
	BPCGEN_INDENT_INC;
	for (cursor = variables; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_VARIABLE* variable = (BPCSMTV_VARIABLE*)cursor->data;

		// offset assign
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "[FieldOffset(%" PRIu32 ")]", offsets);

		// variable spec
		BPCGEN_INDENT_PRINT;
		if (variable->variable_array->is_array) {
			// check requirement
			g_assert(variable->variable_array->is_static_array);

			fprintf(fs, "[MarshalAs(UnmanagedType.ByValArray, SizeConst = %" PRIu32 ")]", variable->variable_array->static_array_len);
		} else {
			if (variable->variable_type->full_uncover_is_basic_type) {
				fprintf(fs, "[MarshalAs(UnmanagedType.%s)]", csharp_unmanaged_type[(size_t)variable->variable_type->full_uncover_basic_type]);
			} else {

			}
		}
	}
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);
}

static void write_enum(FILE* fs, BPCSMTV_ENUM* smtv_enum, BPCGEN_INDENT_TYPE indent) {
	BPCGEN_INDENT_INIT_REF(fs, indent);
	GSList* cursor;

	BPCGEN_INDENT_PRINT;
	fprintf(fs, "public enum %s : %s {", smtv_enum->enum_name, csharp_basic_type[(size_t)smtv_enum->enum_basic_type]); BPCGEN_INDENT_INC;
	for (cursor = smtv_enum->enum_body; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_ENUM_MEMBER* data = (BPCSMTV_ENUM_MEMBER*)cursor->data;

		BPCGEN_INDENT_PRINT;
		if (data->distributed_value_is_uint) {
			fprintf(fs, "%s = %" PRIu64, data->enum_member_name, data->distributed_value.value_uint);
		} else {
			fprintf(fs, "%s = %" PRIi64, data->enum_member_name, data->distributed_value.value_int);
		}
		if (cursor->next != NULL) {
			fputc(',', fs);
		}
	}
	// enum is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

}

void codecs_write_document(FILE* fs, BPCSMTV_DOCUMENT* document) {

}
