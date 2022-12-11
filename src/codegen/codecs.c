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
static char* generate_primitive_type_default(BPCSMTV_VARIABLE* variable) {
	if (variable->variable_type->is_basic_type) {
		return g_strdup(csharp_default_basic_type[variable->variable_type->type_data.basic_type]);
	} else {
		BPCSMTV_PROTOCOL_BODY* body = bpcsmtv_registery_identifier_get(variable->variable_type->semi_uncover_custom_type);

		switch (body->node_type) {
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS:
				return g_strdup(csharp_default_basic_type[body->node_data.alias_data->basic_type]);
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM:
				return g_strdup_printf("%s.%s",		// get first member as default value
					body->node_data.enum_data->enum_name,
					((BPCSMTV_ENUM_MEMBER*)body->node_data.enum_data->enum_body->data)->enum_member_name);
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT:
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG:
			default:
				g_assert_not_reached();
		}
	}
}

//static void write_natural_struct(FILE* fs, BPCSMTV_STRUCT_MODIFIER modifier, GSList* variables, BPCGEN_INDENT_TYPE indent) {
//	// check requirement
//	g_assert(!modifier.is_narrow);
//
//	// generate internal struct
//	// Ref: https://learn.microsoft.com/en-us/dotnet/framework/interop/marshalling-classes-structures-and-unions
//	BPCGEN_INDENT_INIT_REF(fs, indent);
//	GSList* cursor;
//	uint32_t offsets = UINT32_C(0);
//
//	BPCGEN_INDENT_PRINT;
//	fprintf(fs, "[StructLayout(LayoutKind.Explicit, Size = %" PRIu32 ")]", modifier.struct_size);
//	BPCGEN_INDENT_PRINT;
//	fputs("public struct _NaturalStruct {", fs);
//	
//	BPCGEN_INDENT_INC;
//	for (cursor = variables; cursor != NULL; cursor = cursor->next) {
//		BPCSMTV_VARIABLE* variable = (BPCSMTV_VARIABLE*)cursor->data;
//
//		// offset assign
//		BPCGEN_INDENT_PRINT;
//		fprintf(fs, "[FieldOffset(%" PRIu32 ")]", offsets);
//
//		// variable spec
//		BPCGEN_INDENT_PRINT;
//		if (variable->variable_array->is_array) {
//			// check requirement
//			g_assert(variable->variable_array->is_static_array);
//
//			fprintf(fs, "[MarshalAs(UnmanagedType.ByValArray, SizeConst = %" PRIu32 ")]", variable->variable_array->static_array_len);
//		} else {
//			if (variable->variable_type->full_uncover_is_basic_type) {
//				fprintf(fs, "[MarshalAs(UnmanagedType.%s)]", csharp_unmanaged_type[(size_t)variable->variable_type->full_uncover_basic_type]);
//			} else {
//
//			}
//		}
//	}
//	BPCGEN_INDENT_DEC;
//	BPCGEN_INDENT_PRINT;
//	fputc('}', fs);
//}

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
	GString* oper = g_string_new(NULL);
	BPCGEN_INDENT_INIT_REF(fs, indent);

	// get variables detailed type. use NONE to avoid any bonding.
	GSList* bond_vars = bpcgen_constructor_bond_vars(variables, BPCGEN_VARTYPE_NONE);

	// class header
	BPCGEN_INDENT_PRINT;
	if (is_msg) {
		fprintf(fs, "public class %s : _BpMessage {", struct_like_name);
	} else {
		fprintf(fs, "public class %s {", struct_like_name);
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
					char* type_default = generate_primitive_type_default(vardata);
					fprintf(fs, "this.%s = %s;", vardata->variable_name, type_default);
					g_free(type_default);
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
					fprintf(fs, "this.%s = new List<%s>()", vardata->variable_name, get_primitive_type_name(vardata));
					break;
				}

				// narrow and natural struct are processed together
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s = new %s();", vardata->variable_name, get_primitive_type_name(vardata));
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
					fprintf(fs, "this.%s[_i] = new %s();");
					BPCGEN_INDENT_DEC;
					BPCGEN_INDENT_PRINT;
					fputc('}', fs);

					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "this.%s = new List<%s>();", vardata->variable_name, get_primitive_type_name(vardata));
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
	fputs("public override void Deserialize(BinaryReader _ms) {", fs); BPCGEN_INDENT_INC;
	if (is_msg) {
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "if (_ms._BpReadUInt32() != %" PRIu32 ") {", union_data->pMsg->msg_index); BPCGEN_INDENT_INC;
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
				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_ms._BpRead%s();",
						vardata->variable_name,
						csharp_formal_basic_type[vardata->variable_type->full_uncover_basic_type],
						python_struct_fmt_len[vardata->variable_type->full_uncover_basic_type]);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = list(struct.unpack('<%" PRIu32 "%c', _ss.read(%" PRIu32 ")))",
						vardata->variable_name,
						vardata->variable_array->static_array_len,
						python_struct_fmt[vardata->variable_type->full_uncover_basic_type],
						python_struct_fmt_len[vardata->variable_type->full_uncover_basic_type] * vardata->variable_array->static_array_len);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fputs("(_count, ) = _listlen_packer.unpack(_ss.read(_listlen_packer.size))", fs);

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = list(struct.unpack(f'<{_count:d}%c', _ss.read(%" PRIu32 " * _count)))",
						vardata->variable_name,
						python_struct_fmt[vardata->variable_type->full_uncover_basic_type],
						python_struct_fmt_len[vardata->variable_type->full_uncover_basic_type]);

					break;
				}

				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = self._read_bp_string(_ss)", vardata->variable_name);
					break;
				}
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for _i in range(%" PRIu32 "):", vardata->variable_array->static_array_len); BPCGEN_INDENT_INC;

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s[_i] = self._read_bp_string(_ss)", vardata->variable_name);

					BPCGEN_INDENT_DEC;
					break;
				}
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s.clear()", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fputs("(_count, ) = _listlen_packer.unpack(_ss.read(_listlen_packer.size))", fs);

					BPCGEN_INDENT_PRINT;
					fputs("for _i in range(_count):", fs); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s.append(self._read_bp_string(_ss))", vardata->variable_name);
					BPCGEN_INDENT_DEC;

					break;
				}

				// natural and narrow is shared
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s.Deserialize(_ss)", vardata->variable_name);

					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for _i in range(%" PRIu32 "):", vardata->variable_array->static_array_len); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s[_i].Deserialize(_ss)", vardata->variable_name);
					BPCGEN_INDENT_DEC;
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s.clear()", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fputs("(_count, ) = _listlen_packer.unpack(_ss.read(_listlen_packer.size))", fs);
					BPCGEN_INDENT_PRINT;
					fputs("for _i in range(_count):", fs); BPCGEN_INDENT_INC;

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_cache = %s()", vardata->variable_type->type_data.custom_type);
					BPCGEN_INDENT_PRINT;
					fputs("_cache.Deserialize(_ss)", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s.append(_cache)", vardata->variable_name);

					BPCGEN_INDENT_DEC;
					break;
				}

				default:
					g_assert_not_reached();
			}

			// align data
			if (vardata->variable_align->use_align) {
				BPCGEN_INDENT_PRINT;
				fprintf(fs, "_ss.read(%" PRIu32 ")", vardata->variable_align->padding_size);
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
	fprintf(fs, "def Serialize(self, _ss: io.BytesIO):"); BPCGEN_INDENT_INC;
	if (is_msg) {
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "_ss.write(_opcode_packer.pack(%" PRIu32 "))", union_data->pMsg->msg_index);
	}
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;


		if (data->is_bonded) {
			GPtrArray* param_list = constructor_param_list(data);

			// annotation
			BPCGEN_INDENT_PRINT;
			fputs("# ", fs);
			write_args_series(fs, param_list);

			// binary writer
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "_ss.write(%s._struct_packer[%" PRIu32 "].pack(", struct_like_name, preset_struct_index);
			write_args_series(fs, param_list);
			fputs("))", fs);

			destructor_param_list(param_list);
			++preset_struct_index;

		} else {
			// normal process
			BPCSMTV_VARIABLE* vardata = data->plist_vars[0];

			// annotation
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "# %s", vardata->variable_name);

			// body
			switch (data->vars_type[0]) {
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_ss.write(struct.pack('<%c', self.%s))",
						python_struct_fmt[vardata->variable_type->full_uncover_basic_type],
						vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_ss.write(struct.pack('<%" PRIu32 "%c', *self.%s))",
						vardata->variable_array->static_array_len,
						python_struct_fmt[vardata->variable_type->full_uncover_basic_type],
						vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_count = len(self.%s)", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fputs("_ss.write(_listlen_packer.pack(_count))", fs);

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_ss.write(struct.pack(f'<{_count:d}%c', *self.%s))",
						python_struct_fmt[vardata->variable_type->full_uncover_basic_type],
						vardata->variable_name);

					break;
				}

				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self._write_bp_string(_ss, self.%s)", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for _i in range(%" PRIu32 "):", vardata->variable_array->static_array_len); BPCGEN_INDENT_INC;

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self._write_bp_string(_ss, self.%s[_i])", vardata->variable_name);

					BPCGEN_INDENT_DEC;
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_count = len(self.%s)", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fputs("_ss.write(_listlen_packer.pack(_count))", fs);

					BPCGEN_INDENT_PRINT;
					fputs("for _i in range(_count):", fs); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self._write_bp_string(_ss, self.%s[_i])", vardata->variable_name);
					BPCGEN_INDENT_DEC;

					break;
				}

				// natural and narrow is shared
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s.Serialize(_ss)", vardata->variable_name);

					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for _i in range(%" PRIu32 "):", vardata->variable_array->static_array_len); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s[_i].Serialize(_ss)", vardata->variable_name);
					BPCGEN_INDENT_DEC;
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_count = len(self.%s)", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fputs("_ss.write(_listlen_packer.pack(_count))", fs);

					BPCGEN_INDENT_PRINT;
					fputs("for _i in range(_count):", fs); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s[_i].Serialize(_ss)", vardata->variable_name);
					BPCGEN_INDENT_DEC;
					break;
				}

				default:
					g_assert_not_reached();
			}

			// align padding
			if (vardata->variable_align->use_align) {
				BPCGEN_INDENT_PRINT;
				fprintf(fs, "_ss.write(b'\\0' * %" PRIu32 ")", vardata->variable_align->padding_size);
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
	g_string_free(oper, true);
	bpcgen_destructor_bond_vars(bond_vars);
}


void codecs_write_document(FILE* fs, BPCSMTV_DOCUMENT* document) {

}
