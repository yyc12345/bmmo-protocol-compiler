#include "../bpc_code_gen.h"

static const char python_struct_fmt[] = {
	'f', 'd', 'b', 'h', 'i', 'q', 'B', 'H', 'I', 'Q'
};
static const uint32_t python_struct_fmt_len[] = {
	UINT32_C(4), UINT32_C(8), UINT32_C(1), UINT32_C(2), UINT32_C(4), UINT32_C(8), UINT32_C(1), UINT32_C(2), UINT32_C(4), UINT32_C(8)
};
static const char* python_struct_default[] = {
	"0.0", "0.0", "0", "0", "0", "0", "0", "0", "0", "0"
};

static void write_tuple_series(FILE* fs, GPtrArray* arr) {
	guint c;
	for (c = 0u; c < arr->len; ++c) {
		fputs((gchar*)(arr->pdata[c]), fs);
		fputs(", ", fs);	// in any cases, always append splittor.
	}
}
static void write_args_series(FILE* fs, GPtrArray* arr) {
	guint c;
	for (c = 0u; c < arr->len; ++c) {
		// append splittor before entry only when non-first node.
		if (c != 0u)
			fputs(", ", fs);

		fputs((gchar*)(arr->pdata[c]), fs);
	}
}

static gchar* generate_pack_fmt(BOND_VARS* bond_vars) {
	GString* pack_fmt = g_string_new(NULL);
	uint32_t c;

	// only bonded vars need calling this functions
	g_assert(bond_vars->is_bonded);
	for (c = 0; c < bond_vars->bond_vars_len; ++c) {
		// only single primitive can be bonded
		g_assert(bond_vars->vars_type[c] == BPCGEN_VARTYPE_SINGLE_PRIMITIVE);
		g_assert(bond_vars->plist_vars[c]->variable_type->full_uncover_basic_type != BPCSMTV_BASIC_TYPE_STRING);

		// core convertion
		g_string_append_c(pack_fmt, python_struct_fmt[bond_vars->plist_vars[c]->variable_type->full_uncover_basic_type]);
		// variable padding
		if (bond_vars->plist_vars[c]->variable_align->use_align) {
			g_string_append_printf(pack_fmt, "%" PRIu32 "x", bond_vars->plist_vars[c]->variable_align->padding_size);
		}

	}

	return g_string_free(pack_fmt, false);
}
static GPtrArray* constructor_param_list(BOND_VARS* bond_vars) {
	GPtrArray* param_list = g_ptr_array_new_with_free_func(g_free);
	uint32_t c;

	g_assert(bond_vars->is_bonded);
	for (c = 0; c < bond_vars->bond_vars_len; ++c) {
		g_assert(bond_vars->vars_type[c] == BPCGEN_VARTYPE_SINGLE_PRIMITIVE);

		g_ptr_array_add(param_list, g_strdup(bond_vars->plist_vars[c]->variable_name));
	}

	return param_list;
}
static void destructor_param_list(GPtrArray* param_list) {
	g_ptr_array_free(param_list, true);
}

static void write_enum(FILE* fs, BPCSMTV_ENUM* smtv_enum) {
	GSList* cursor;
	BPCGEN_INDENT_INIT_NEW(fs);

	BPCGEN_INDENT_PRINT;
	fprintf(fs, "class %s(enum.IntEnum):", smtv_enum->enum_name); BPCGEN_INDENT_INC;
	for (cursor = smtv_enum->enum_body; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_ENUM_MEMBER* data = (BPCSMTV_ENUM_MEMBER*)cursor->data;

		BPCGEN_INDENT_PRINT;
		if (data->distributed_value_is_uint) {
			fprintf(fs, "%s = %" PRIu64, data->enum_member_name, data->distributed_value.value_uint);
		} else {
			fprintf(fs, "%s = %" PRIi64, data->enum_member_name, data->distributed_value.value_int);
		}
	}
	// enum is over
	BPCGEN_INDENT_DEC;
}

static void write_struct_or_msg(FILE* fs, BPCGEN_STRUCT_LIKE* union_data, bool is_msg) {
	GSList* cursor = NULL, * variables = (is_msg ? union_data->pMsg->msg_body : union_data->pStruct->struct_body);
	BPCSMTV_STRUCT_MODIFIER* modifier = (is_msg ? union_data->pMsg->msg_modifier : union_data->pStruct->struct_modifier);
	char* struct_like_name = (is_msg ? union_data->pMsg->msg_name : union_data->pStruct->struct_name);
	GString* oper = g_string_new(NULL);
	BPCGEN_INDENT_INIT_NEW(fs);

	// get bond variables. python only allow combineing single primitive variables
	GSList* bond_vars = bpcgen_constructor_bond_vars(variables, BPCGEN_VARTYPE_SINGLE_PRIMITIVE);

	// class header
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "class %s(%s):", struct_like_name,
		(is_msg ? "_BpMessage" : "object"));
	BPCGEN_INDENT_INC;

	// static struct pack for combined nodes
	BPCGEN_INDENT_PRINT;
	fputs("_struct_packer = (", fs);
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		if (data->is_bonded) {
			gchar* fmt_str = generate_pack_fmt(data);
			fprintf(fs, "struct.Struct('<%s'), ", fmt_str);
			g_free(fmt_str);
		}
	}
	fputs(")", fs);

	// class constructor
	BPCGEN_INDENT_PRINT;
	fputs("def __init__(self):", fs); BPCGEN_INDENT_INC;
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			BPCSMTV_VARIABLE* vardata = data->plist_vars[c];
			switch (data->vars_type[c]) {
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = %s", vardata->variable_name, python_struct_default[vardata->variable_type->full_uncover_basic_type]);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = [%s] * %" PRIu32,
						vardata->variable_name,
						python_struct_default[vardata->variable_type->full_uncover_basic_type],
						vardata->variable_array->static_array_len);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = []", vardata->variable_name);
					break;
				}

				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = \"\"", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = [\"\"] * %" PRIu32, vardata->variable_name, vardata->variable_array->static_array_len);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = []", vardata->variable_name);
					break;
				}

				// natural and narrow is shared
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = %s()", vardata->variable_name, vardata->variable_type->type_data.custom_type);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = list(%s() for i in range(%" PRIu32 "))",
						vardata->variable_name,
						vardata->variable_type->type_data.custom_type,
						vardata->variable_array->static_array_len);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = []", vardata->variable_name);
					break;
				}

				default:
					g_assert_not_reached();
			}

		}
	}
	if (bond_vars == NULL) {
		// if there are no member, wee need write extra `pass`
		BPCGEN_INDENT_PRINT;
		fputs("pass", fs);
	}
	BPCGEN_INDENT_DEC;

	// reliablility getter and opcode getter for msg
	if (is_msg) {
		// reliable
		BPCGEN_INDENT_PRINT;
		fputs("def GetIsReliable(self) -> bool:", fs); BPCGEN_INDENT_INC;

		BPCGEN_INDENT_PRINT;
		if (modifier->is_reliable) fputs("return True", fs);
		else fputs("return False", fs);
		BPCGEN_INDENT_DEC;

		// opcode
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "def GetOpcode(self) -> int:"); BPCGEN_INDENT_INC;
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "return %" PRIu32, union_data->pMsg->msg_index);
		BPCGEN_INDENT_DEC;
	}

	// deserialize func
	uint32_t preset_struct_index = UINT32_C(0);
	BPCGEN_INDENT_PRINT;
	fputs("def Deserialize(self, _ss: io.BytesIO):", fs); BPCGEN_INDENT_INC;
	if (is_msg) {
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "if _opcode_packer.unpack(_ss.read(_opcode_packer.size))[0] != %" PRIu32 ":", union_data->pMsg->msg_index); BPCGEN_INDENT_INC;
		BPCGEN_INDENT_PRINT;
		fputs("raise Exception('Invalid opcode!')", fs);
		BPCGEN_INDENT_DEC;
	}
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;

		if (data->is_bonded) {
			// bonded variables
			// get param list
			GPtrArray* param_list = constructor_param_list(data);

			// annotation
			BPCGEN_INDENT_PRINT;
			fputs("# ", fs);
			write_args_series(fs, param_list);

			// binary reader
			BPCGEN_INDENT_PRINT;
			fputc('(', fs);
			write_tuple_series(fs, param_list);
			fprintf(fs, ") = %s._struct_packer[%" PRIu32 "].unpack(_ss.read(%s._struct_packer[%" PRIu32 "].size))",
				struct_like_name,
				preset_struct_index,
				struct_like_name,
				preset_struct_index);

			// free param list
			destructor_param_list(param_list);

			// inc index
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
					fprintf(fs, "(self.%s, ) = struct.unpack('<%c', _ss.read(%" PRIu32 "))",
						vardata->variable_name,
						python_struct_fmt[vardata->variable_type->full_uncover_basic_type],
						python_struct_fmt_len[vardata->variable_type->full_uncover_basic_type]);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
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

				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = self._read_bp_string(_ss)", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for _i in range(%" PRIu32 "):", vardata->variable_array->static_array_len); BPCGEN_INDENT_INC;

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s[_i] = self._read_bp_string(_ss)", vardata->variable_name);

					BPCGEN_INDENT_DEC;
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
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
	if (bond_vars == NULL && (!is_msg)) {
		// no variable in struct, write pass
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "pass");
	}
	BPCGEN_INDENT_DEC;

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
	if (bond_vars == NULL && (!is_msg)) {
		// no variables in struct, write pass
		BPCGEN_INDENT_PRINT;
		fputs("pass", fs);
	}
	// func deserialize is over
	BPCGEN_INDENT_DEC;

	// class define is over
	BPCGEN_INDENT_DEC;

	// free all cache data
	g_string_free(oper, true);
	bpcgen_destructor_bond_vars(bond_vars);
}

static void write_opcode_enum(FILE* fs, GSList* msg_ls) {
	GSList* cursor;
	BPCGEN_INDENT_INIT_NEW(fs);

	// write opcode enum
	BPCGEN_INDENT_PRINT;
	fputs("class _OpCode(enum.IntEnum):", fs); BPCGEN_INDENT_INC;
	for (cursor = msg_ls; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_MSG* data = (BPCSMTV_MSG*)cursor->data;

		BPCGEN_INDENT_PRINT;
		fprintf(fs, "%s = %" PRIu32, data->msg_name, data->msg_index);
	}
	if (msg_ls == NULL) {
		fputs("pass", fs);
	}
	// class opcode is over
	BPCGEN_INDENT_DEC;
}

static void write_uniform_deserialize(FILE* fs, GSList* msg_ls) {
	GSList* cursor;
	bool is_first = true;
	BPCGEN_INDENT_INIT_NEW(fs);

	// write uniformed deserialize func
	BPCGEN_INDENT_PRINT;
	fputs("def _UniformDeserialize(_ss: io.BytesIO) -> _BpMessage:", fs); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fputs("_opcode = _opcode_packer.unpack(_ss.read(_opcode_packer.size))[0]", fs);
	for (cursor = msg_ls; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_MSG* data = (BPCSMTV_MSG*)cursor->data;

		BPCGEN_INDENT_PRINT;
		// print if or elif statements
		if (is_first) is_first = false;
		else fputs("el", fs);
		fprintf(fs, "if _opcode == _OpCode.%s:", data->msg_name);

		// write if body
		BPCGEN_INDENT_INC;
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "_data = %s()", data->msg_name);
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "_data.Deserialize(_ss)");
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "return _data");
		BPCGEN_INDENT_DEC;
	}
	// default return
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "return None");
	// uniform func is over
	BPCGEN_INDENT_DEC;

}

void codepy_write_document(FILE* fs, BPCSMTV_DOCUMENT* document) {
	// write header
	bpcfs_write_snippets(fs, &bpcsnp_py_header);

	// iterate list to get data
	// and pick msg
	GSList* cursor = NULL;
	BPCGEN_STRUCT_LIKE struct_like = { 0 };
	for (cursor = document->protocol_body; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_PROTOCOL_BODY* data = (BPCSMTV_PROTOCOL_BODY*)cursor->data;

		switch (data->node_type) {
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM:
				write_enum(fs, data->node_data.enum_data);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT:
				struct_like.pStruct = data->node_data.struct_data;
				write_struct_or_msg(fs, &struct_like, false);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG:
				struct_like.pMsg = data->node_data.msg_data;
				write_struct_or_msg(fs, &struct_like, true);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS:
				break;
			default:
				g_assert_not_reached();
		}
	}

	// write some tails
	GSList* msg_ls = bpcgen_constructor_msg_list(document->protocol_body);
	write_opcode_enum(fs, msg_ls);
	write_uniform_deserialize(fs, msg_ls);

	// free msg list
	bpcgen_destructor_msg_list(msg_ls);

}
