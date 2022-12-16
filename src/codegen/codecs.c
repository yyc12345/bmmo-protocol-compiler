#include "../bpc_code_gen.h"

static const char* csharp_basic_type[] = {
	"float", "double", "sbyte", "short", "int", "long", "byte", "ushort", "uint", "ulong", "string"
};
static const char* csharp_formal_basic_type[] = {
	"Single", "Double", "SByte", "Int16", "Int32", "Int64", "Byte", "UInt16", "UInt32", "UInt64", "String"
};
static const char* csharp_default_basic_type[] = {
	"0.0f", "0.0", "0", "0", "0", "0", "0", "0", "0", "0", "\"\""
};
//// Ref: https://learn.microsoft.com/en-us/dotnet/api/system.runtime.interopservices.unmanagedtype?view=net-6.0
//static const char* csharp_unmanaged_type[] = {
//	"R4", "R8", "I1", "I2", "I4", "I8", "U1", "U2", "U4", "U8", "LPUTF8Str"	// the unmanaged type of string still is invalid. LPUTF8Str just is a placeholder.
//};

static char* get_primitive_type_name(BPCSMTV_VARIABLE* variable) {
	return (variable->variable_type->semi_uncover_is_basic_type ?
		csharp_basic_type[variable->variable_type->full_uncover_basic_type] :
		variable->variable_type->semi_uncover_custom_type);
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


static void write_struct_or_msg(FILE* fs, BPCGEN_STRUCT_LIKE* union_data, bool is_msg, BPCGEN_INDENT_TYPE indent) {
	GSList* cursor = NULL, * variables = (is_msg ? union_data->pMsg->msg_body : union_data->pStruct->struct_body);
	BPCSMTV_STRUCT_MODIFIER* modifier = (is_msg ? union_data->pMsg->msg_modifier : union_data->pStruct->struct_modifier);
	char* struct_like_name = (is_msg ? union_data->pMsg->msg_name : union_data->pStruct->struct_name);
	BPCGEN_INDENT_INIT_REF(fs, indent);

	// get variables detailed type. use NONE to avoid any bonding.
	GSList* bond_vars = bpcgen_constructor_bond_vars(variables, BPCGEN_VARTYPE_NONE);

	// class header
	BPCGEN_INDENT_PRINT;
	if (is_msg) {
		fprintf(fs, "public class %s : _BpMessage {", struct_like_name);
	} else {
		fprintf(fs, "public class %s : _BpStruct {", struct_like_name);
	}
	BPCGEN_INDENT_INC;

	// class variable define
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			BPCSMTV_VARIABLE* vardata = data->plist_vars[c];
			switch (data->vars_type[c]) {
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_STRING:
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "public %s %s;", get_primitive_type_name(vardata), vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "public %s[] %s;", get_primitive_type_name(vardata), vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "public List<%s> %s;", get_primitive_type_name(vardata), vardata->variable_name);
					break;
				}

				default:
					g_assert_not_reached();
			}
		}
	}

	// class constructor
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "public %s() {", struct_like_name); BPCGEN_INDENT_INC;
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			BPCSMTV_VARIABLE* vardata = data->plist_vars[c];
			switch (data->vars_type[c]) {
				// basic types are processed together
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s = default(%s);", vardata->variable_name, get_primitive_type_name(vardata));
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s = new %s[%" PRIu32 "];",
						vardata->variable_name,
						get_primitive_type_name(vardata),
						vardata->variable_array->static_array_len);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s = new List<%s>();", vardata->variable_name, get_primitive_type_name(vardata));
					break;
				}

				// narrow and natural struct are processed together
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s = new %s();", vardata->variable_name, vardata->variable_type->type_data.custom_type);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s = new %s[%" PRIu32 "];",
						vardata->variable_name,
						get_primitive_type_name(vardata),
						vardata->variable_array->static_array_len);

					// loop to init each item.
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (int _i = 0; i < this.%s.Length; ++i) {", vardata->variable_name); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s[_i] = new %s();", vardata->variable_name, vardata->variable_type->type_data.custom_type);
					BPCGEN_INDENT_DEC;
					BPCGEN_INDENT_PRINT;
					fputc('}', fs);

					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s = new List<%s>();", vardata->variable_name, vardata->variable_type->type_data.custom_type);
					break;
				}

				default:
					g_assert_not_reached();
			}

		}
	}
	// constructor is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// reliablility getter and opcode getter for msg
	if (is_msg) {
		// reliable
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "public override bool GetIsReliable() => %s;",
			(modifier->is_reliable ? "true" : "false"));

		// opcode
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "public override _OpCode GetOpCode() => _OpCode.%s;", struct_like_name); BPCGEN_INDENT_INC;
	}

	// deserialize func
	uint32_t preset_struct_index = UINT32_C(0);
	BPCGEN_INDENT_PRINT;
	fputs("public override void Deserialize(BinaryReader _br) {", fs); BPCGEN_INDENT_INC;
	if (is_msg) {
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "if (_br._BpReadOpCode() != _OpCode.%s) {", struct_like_name); BPCGEN_INDENT_INC;
		BPCGEN_INDENT_PRINT;
		fputs("throw new Exception(\"Invalid OpCode!\");", fs);
		BPCGEN_INDENT_DEC;
		BPCGEN_INDENT_PRINT;
		fputc('}', fs);
	}
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			BPCSMTV_VARIABLE* vardata = data->plist_vars[c];

			// annotation
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "// %s", vardata->variable_name);

			// body
			switch (data->vars_type[0]) {
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					if (vardata->variable_type->semi_uncover_is_basic_type) {
						// basic type, alias
						fprintf(fs, "this.%s = _br._BpRead%s();",
							vardata->variable_name,
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type]);
					} else {
						// enum
						fprintf(fs, "this.%s = (%s)_br._BpRead%s();",
							vardata->variable_name,
							vardata->variable_type->semi_uncover_custom_type,
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type]);
					}
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					if (vardata->variable_type->semi_uncover_is_basic_type) {
						// basic type, alias
						fprintf(fs, "this.%s = _br._BpRead%sArray(%" PRIu32 ");",
							vardata->variable_name,
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type],
							vardata->variable_array->static_array_len);
					} else {
						// enum
						fprintf(fs, "this.%s = _Helper._CastIntArray2EnumArray<%s, %s>(_br._BpRead%sArray(%" PRIu32 "));",
							vardata->variable_name,
							vardata->variable_type->semi_uncover_custom_type,
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type],
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type],
							vardata->variable_array->static_array_len);
					}
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s.Clear();", vardata->variable_name);

					BPCGEN_INDENT_PRINT;
					if (vardata->variable_type->semi_uncover_is_basic_type) {
						// basic type, alias
						fprintf(fs, "this.%s.AddRange(_br._BpRead%sArray((int)_br._BpReadUInt32()));",
							vardata->variable_name,
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type]);
					} else {
						// enum
						fprintf(fs, "this.%s.AddRange(_Helper._CastIntArray2EnumArray<%s, %s>(_br._BpRead%sArray((int)_br._BpReadUInt32())));",
							vardata->variable_name,
							vardata->variable_type->semi_uncover_custom_type,
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type],
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type]);
					}
					break;
				}

				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s =_br._BpReadString();", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (int _i = 0; _i < %" PRIu32 "; ++_i) {", vardata->variable_array->static_array_len); BPCGEN_INDENT_INC;

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s[_i] = _br._BpReadString();", vardata->variable_name);

					BPCGEN_INDENT_DEC;
					BPCGEN_INDENT_PRINT;
					fputc('}', fs);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s.Clear();", vardata->variable_name);

					BPCGEN_INDENT_PRINT;
					fputs("for (int _i = 0, _count = (int)_br._BpReadUInt32(); _i < _count; ++_i) {", fs); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s.Add(_br._BpReadString());", vardata->variable_name);
					BPCGEN_INDENT_DEC;
					BPCGEN_INDENT_PRINT;
					fputc('}', fs);
					break;
				}

				// natural and narrow is shared
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s.Deserialize(_br);", vardata->variable_name);

					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (int _i = 0; _i < %" PRIu32 "; ++_i) {", vardata->variable_array->static_array_len); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s[_i].Deserialize(_br);", vardata->variable_name);
					BPCGEN_INDENT_DEC;
					BPCGEN_INDENT_PRINT;
					fputc('}', fs);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s.Clear();", vardata->variable_name);

					BPCGEN_INDENT_PRINT;
					fputs("for (int _i = 0, _count = (int)_br._BpReadUInt32(); _i < _count; ++_i) {", fs); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "var _cache = new %s();", vardata->variable_type->type_data.custom_type);
					BPCGEN_INDENT_PRINT;
					fputs("cache.Deserialize(_br);", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s.Add(_cache);", vardata->variable_name);
					BPCGEN_INDENT_DEC;
					BPCGEN_INDENT_PRINT;
					fputc('}', fs);
					break;
				}

				default:
					g_assert_not_reached();
			}

			// align data
			if (vardata->variable_align->use_align) {
				BPCGEN_INDENT_PRINT;
				fprintf(fs, "br.BaseStream.Seek(%" PRIu32 ", SeekOrigin.Current);", vardata->variable_align->padding_size);
			}
		}
	}
	// deserialize is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// serialize func
	preset_struct_index = UINT32_C(0);
	BPCGEN_INDENT_PRINT;
	fputs("public override void Serialize(BinaryWriter _bw) {", fs); BPCGEN_INDENT_INC;
	if (is_msg) {
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "_bw._BpWriteOpCode(_OpCode.%s);", struct_like_name);
	}
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			BPCSMTV_VARIABLE* vardata = data->plist_vars[c];

			// annotation
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "// %s", vardata->variable_name);

			// body
			switch (data->vars_type[0]) {
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					if (vardata->variable_type->semi_uncover_is_basic_type) {
						// basic type, alias
						fprintf(fs, "_bw._BpWrite%s(this.%s);",
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type],
							vardata->variable_name);
					} else {
						//enum
						fprintf(fs, "_bw._BpWrite%s((%s)this.%s);",
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type],
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type],
							vardata->variable_name);
					}
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					if (vardata->variable_type->semi_uncover_is_basic_type) {
						// basic type, alias
						fprintf(fs, "_bw._BpWrite%sArray(ref this.%s);",
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type],
							vardata->variable_name);
					} else {
						//enum
						fprintf(fs, "_bw._BpWrite%sArray(ref _Helper._CastEnumArray2IntArray<%s, %s>(this.%s));",
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type],
							vardata->variable_type->semi_uncover_custom_type,
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type],
							vardata->variable_name);
					}
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_bw._BpWriteUInt32((UInt32)this.%s.Count);", vardata->variable_name);

					BPCGEN_INDENT_PRINT;
					if (vardata->variable_type->semi_uncover_is_basic_type) {
						// basic type, alias
						fprintf(fs, "_bw._BpWrite%sArray(ref this.%s.ToArray());",
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type],
							vardata->variable_name);
					} else {
						//enum
						fprintf(fs, "_bw._BpWrite%sArray(ref _Helper._CastEnumArray2IntArray<%s, %s>(this.%s.ToArray()));",
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type],
							vardata->variable_type->semi_uncover_custom_type,
							csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type],
							vardata->variable_name);
					}
					break;
				}

				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_bw._BpWriteString(this.%s);", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (int _i = 0; _i < %" PRIu32 "; ++_i) {", vardata->variable_array->static_array_len); BPCGEN_INDENT_INC;
					
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_bw._BpWriteString(this.%s[_i]);", vardata->variable_name);

					BPCGEN_INDENT_DEC;
					BPCGEN_INDENT_PRINT;
					fputc('}', fs);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_bw._BpWriteUInt32((UInt32)this.%s.Count);", vardata->variable_name);

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (int _i = 0; _i < this.%s.Count; ++_i) {", vardata->variable_name); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_bw._BpWriteString(this.%s[_i]);", vardata->variable_name);
					BPCGEN_INDENT_DEC;
					BPCGEN_INDENT_PRINT;
					fputc('}', fs);

					break;
				}

				// natural and narrow is shared
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s.Serialize(_bw);", vardata->variable_name);

					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (int _i = 0; _i < %" PRIu32 "; ++_i) {", vardata->variable_array->static_array_len); BPCGEN_INDENT_INC;

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s[_i].Serialize(_bw);", vardata->variable_name);

					BPCGEN_INDENT_DEC;
					BPCGEN_INDENT_PRINT;
					fputc('}', fs);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_bw._BpWriteUInt32((UInt32)this.%s.Count);", vardata->variable_name);

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (int _i = 0; _i < this.%s.Count; ++_i) {", vardata->variable_name); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s[_i].Serialize(_bw);", vardata->variable_name);
					BPCGEN_INDENT_DEC;
					BPCGEN_INDENT_PRINT;
					fputc('}', fs);
					break;
				}

				default:
					g_assert_not_reached();
			}

			// align padding
			if (vardata->variable_align->use_align) {
				BPCGEN_INDENT_PRINT;
				fprintf(fs, "bw.BaseStream.Seek(%" PRIu32 ", SeekOrigin.Current);", vardata->variable_align->padding_size);
			}
		}

	}
	// serialize is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// class define is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// free all cache data
	bpcgen_destructor_bond_vars(bond_vars);
}

static void write_opcode_enum(FILE* fs, GSList* msg_ls, BPCGEN_INDENT_TYPE indent) {
	GSList* cursor;
	BPCGEN_INDENT_INIT_REF(fs, indent);

	// write opcode enum
	BPCGEN_INDENT_PRINT;
	fputs("public enum _OpCode : uint {", fs); BPCGEN_INDENT_INC;
	for (cursor = msg_ls; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_MSG* data = (BPCSMTV_MSG*)cursor->data;

		BPCGEN_INDENT_PRINT;
		fprintf(fs, "%s = %" PRIu32, data->msg_name, data->msg_index);
		if (cursor->next != NULL) {
			fputc(',', fs);
		}
	}

	// class opcode is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);
}

static void write_uniform_deserialize(FILE* fs, GSList* msg_ls, BPCGEN_INDENT_TYPE indent) {
	GSList* cursor;
	BPCGEN_INDENT_INIT_REF(fs, indent);

	// write uniformed deserialize func
	BPCGEN_INDENT_PRINT;
	fputs("public static partial class _Helper {", fs); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fputs("public static _BpMessage UniformDeserialize(BinaryReader _br) {", fs); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fputs("var _opcode = _br._BpPeekOpCode();", fs);
	for (cursor = msg_ls; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_MSG* data = (BPCSMTV_MSG*)cursor->data;

		BPCGEN_INDENT_PRINT;
		fprintf(fs, "case _OpCode.%s: {", data->msg_name);

		// write if body
		BPCGEN_INDENT_INC;
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "var _data = new %s();", data->msg_name);
		BPCGEN_INDENT_PRINT;
		fputs("_data.Deserialize(br);", fs);
		BPCGEN_INDENT_PRINT;
		fputs("return _data;", fs);

		BPCGEN_INDENT_DEC;
		BPCGEN_INDENT_PRINT;
		fputc('}', fs);
	}
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// default return
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "return null;");

	// uniform func is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

}

char* generate_namespaces(GSList* ns_list) {
	GString* strl = g_string_new(NULL);
	GSList* cursor = NULL;

	for (cursor = ns_list; cursor != NULL; cursor = cursor->next) {
		g_string_append(strl, (gchar*)cursor->data);
		if (cursor->next != NULL) {
			g_string_append_c(strl, '.');
		}
	}

	return g_string_free(strl, false);
}

void codecs_write_document(FILE* fs, BPCSMTV_DOCUMENT* document) {
	BPCGEN_INDENT_INIT_NEW(fs);

	// write header
	bpcfs_write_snippets(fs, &bpcsnp_cs_header);

	// write namespace
	char* ns = generate_namespaces(document->namespace_data);
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "namespace %s {", ns); BPCGEN_INDENT_INC;
	g_free(ns);

	// write opcode
	GSList* msg_ls = bpcgen_constructor_msg_list(document->protocol_body);
	write_opcode_enum(fs, msg_ls, BPCGEN_INDENT_REF);

	// write functions snippets
	bpcfs_write_snippets(fs, &bpcsnp_cs_functions);

	// iterate list to get data
	GSList* cursor = NULL;
	BPCGEN_STRUCT_LIKE struct_like = { 0 };
	for (cursor = document->protocol_body; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_PROTOCOL_BODY* data = (BPCSMTV_PROTOCOL_BODY*)cursor->data;

		switch (data->node_type) {
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM:
				write_enum(fs, data->node_data.enum_data, BPCGEN_INDENT_REF);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT:
				struct_like.pStruct = data->node_data.struct_data;
				write_struct_or_msg(fs, &struct_like, false, BPCGEN_INDENT_REF);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG:
				struct_like.pMsg = data->node_data.msg_data;
				write_struct_or_msg(fs, &struct_like, true, BPCGEN_INDENT_REF);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS:
				break;
			default:
				g_assert_not_reached();
		}
	}

	// write uniformed deserializer
	write_uniform_deserialize(fs, msg_ls, BPCGEN_INDENT_REF);

	// free msg list
	bpcgen_destructor_msg_list(msg_ls);

	// namespace over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

}
