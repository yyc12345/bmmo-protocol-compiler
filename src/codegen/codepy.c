#include "../bpc_code_gen.h"

static const char python_struct_fmt[] = {
	'f', 'd', 'b', 'h', 'i', 'q', 'B', 'H', 'I', 'Q'
};
static const char* python_struct_default[] = {
	"0.0", "0.0", "0", "0", "0", "0", "0", "0", "0", "0", "\"\""
};
static const char* python_struct_type_hints[] = {
	"float", "float", "int", "int", "int", "int", "int", "int", "int", "int", "str"
};

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

		g_ptr_array_add(param_list, g_strdup_printf("self.%s", bond_vars->plist_vars[c]->variable_name));
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
		fprintf(fs, "%s: int = ", data->enum_member_name);
		bpcgen_print_enum_member_value(fs, data);
	}
	// enum is over
	BPCGEN_INDENT_DEC;
}

static void write_struct_or_msg(FILE* fs, BPCGEN_STRUCT_LIKE* union_data) {
	GSList* cursor = NULL;
	bool is_msg;  GSList* variables; BPCSMTV_STRUCT_MODIFIER* modifier; char* struct_like_name;
	bpcgen_pick_struct_like_data(union_data, &is_msg, &variables, &modifier, &struct_like_name);
	BPCGEN_INDENT_INIT_NEW(fs);

	// get bond variables. python only allow combineing single primitive variables
	GSList* bond_vars = bpcgen_constructor_bond_vars(variables, BPCGEN_VARTYPE_SINGLE_PRIMITIVE);

	// class header
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "class %s(%s):", struct_like_name,
		(is_msg ? "BpMessage" : "BpStruct"));
	BPCGEN_INDENT_INC;

	// static struct pack for combined primitives and static primitives
	BPCGEN_INDENT_PRINT;
	fputs("_struct_packer: typing.ClassVar[tuple[_pStructStruct]] = (", fs);
	BPCGEN_INDENT_INC;
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;

		if (data->is_bonded) {
			// combined primitives
			gchar* fmt_str = generate_pack_fmt(data);

			BPCGEN_INDENT_PRINT;
			fprintf(fs, "_pStructStruct('<%s'), ", fmt_str);

			g_free(fmt_str);
		} else {
			// static primitives
			if (data->vars_type[0] == BPCGEN_VARTYPE_STATIC_PRIMITIVE) {
				BPCSMTV_VARIABLE* vardata = data->plist_vars[0];

				BPCGEN_INDENT_PRINT;
				fprintf(fs, "_pStructStruct('<%" PRIu32 "%c'), ",
					vardata->variable_array->static_array_len,
					python_struct_fmt[vardata->variable_type->full_uncover_basic_type]
				);
			}
		}
	}
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputs(")", fs);

	// class decl
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			BPCSMTV_VARIABLE* vardata = data->plist_vars[c];
			switch (data->vars_type[c]) {
				// primitive are shared
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s: %s",
						vardata->variable_name,
						python_struct_type_hints[vardata->variable_type->full_uncover_basic_type]
					);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s: list[%s]",
						vardata->variable_name,
						python_struct_type_hints[vardata->variable_type->full_uncover_basic_type]
					);
					break;
				}

				// natural and narrow are shared
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s: %s",
						vardata->variable_name,
						vardata->variable_type->type_data.custom_type
					);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s: list[%s]",
						vardata->variable_name,
						vardata->variable_type->type_data.custom_type
					);
					break;
				}

				default:
					g_assert_not_reached();
			}

		}
	}

	// class constructor
	BPCGEN_INDENT_PRINT;
	fputs("def __init__(self):", fs); BPCGEN_INDENT_INC;
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			BPCSMTV_VARIABLE* vardata = data->plist_vars[c];
			switch (data->vars_type[c]) {
				// primitive and string are shared
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = %s",
						vardata->variable_name,
						python_struct_default[vardata->variable_type->full_uncover_basic_type]);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = [%s] * %" PRIu32,
						vardata->variable_name,
						python_struct_default[vardata->variable_type->full_uncover_basic_type],
						vardata->variable_array->static_array_len);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = []", vardata->variable_name);
					break;
				}

				// natural and narrow are shared
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = %s()",
						vardata->variable_name,
						vardata->variable_type->type_data.custom_type);
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
		fputs("def IsReliable(self) -> bool:", fs); BPCGEN_INDENT_INC;

		BPCGEN_INDENT_PRINT;
		if (modifier->is_reliable) fputs("return True", fs);
		else fputs("return False", fs);
		BPCGEN_INDENT_DEC;

		// opcode
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "def GetOpCode(self) -> int:"); BPCGEN_INDENT_INC;
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "return OpCode.%s", struct_like_name);
		BPCGEN_INDENT_DEC;
	}

	// deserialize func
	uint32_t preset_struct_index = UINT32_C(0);
	BPCGEN_INDENT_PRINT;
	fputs("def Deserialize(self, _ss: io.BytesIO):", fs); BPCGEN_INDENT_INC;
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;

		// annotation
		BPCGEN_INDENT_PRINT;
		bpcgen_print_variables_annotation(fs, "# ", data);

		if (data->is_bonded) {
			// bonded variables
			// get param list
			GPtrArray* param_list = constructor_param_list(data);

			// binary reader
			BPCGEN_INDENT_PRINT;
			fputc('(', fs);
			bpcgen_print_join_ptrarray(fs, ", ", false, param_list);
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

			// body
			switch (data->vars_type[0]) {
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				{
					// static primitive also use _struct_packer
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = list(%s._struct_packer[%" PRIu32 "].unpack(_ss.read(%s._struct_packer[%" PRIu32 "].size)))",
						vardata->variable_name,
						struct_like_name,
						preset_struct_index,
						struct_like_name,
						preset_struct_index
					);

					// inc index
					++preset_struct_index;

					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fputs("(_count, ) = _listlen_packer.unpack(_ss.read(_listlen_packer.size))", fs);

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = list(_pStructUnpack(f'<{_count:d}%c', _ss.read(%" PRIu32 " * _count)))",
						vardata->variable_name,
						python_struct_fmt[vardata->variable_type->full_uncover_basic_type],
						bpcsmtv_get_bt_size(vardata->variable_type->full_uncover_basic_type)
					);

					break;
				}

				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s = self._ReadBpString(_ss)", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for _i in range(%" PRIu32 "):", vardata->variable_array->static_array_len); BPCGEN_INDENT_INC;

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s[_i] = self._ReadBpString(_ss)", vardata->variable_name);

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
					fprintf(fs, "self.%s.append(self._ReadBpString(_ss))", vardata->variable_name);
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

				// these variables has been process in bond_vars
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
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
	if (bond_vars == NULL) {
		// no variable, read 1 byte anyway. because sizeof(empty_struct) == 1u
		BPCGEN_INDENT_PRINT;
		fputs("_ss.read(1)", fs);
	}
	BPCGEN_INDENT_DEC;

	// serialize func
	preset_struct_index = UINT32_C(0);
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "def Serialize(self, _ss: io.BytesIO):"); BPCGEN_INDENT_INC;
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;

		// annotation
		BPCGEN_INDENT_PRINT;
		bpcgen_print_variables_annotation(fs, "# ", data);

		if (data->is_bonded) {
			GPtrArray* param_list = constructor_param_list(data);

			// binary writer
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "_ss.write(%s._struct_packer[%" PRIu32 "].pack(", struct_like_name, preset_struct_index);
			bpcgen_print_join_ptrarray(fs, ", ", true, param_list);
			fputs("))", fs);

			destructor_param_list(param_list);
			++preset_struct_index;

		} else {
			// normal process
			BPCSMTV_VARIABLE* vardata = data->plist_vars[0];

			// body
			switch (data->vars_type[0]) {
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				{
					// static primitive also use _struct_packer
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_ss.write(%s._struct_packer[%" PRIu32 "].pack(*self.%s))",
						struct_like_name,
						preset_struct_index,
						vardata->variable_name
					);

					// inc index
					++preset_struct_index;

					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_count = len(self.%s)", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fputs("_ss.write(_listlen_packer.pack(_count))", fs);

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_ss.write(_pStructPack(f'<{_count:d}%c', *self.%s))",
						python_struct_fmt[vardata->variable_type->full_uncover_basic_type],
						vardata->variable_name);

					break;
				}

				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self._WriteBpString(_ss, self.%s)", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for _i in range(%" PRIu32 "):", vardata->variable_array->static_array_len); BPCGEN_INDENT_INC;

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self._WriteBpString(_ss, self.%s[_i])", vardata->variable_name);

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
					fprintf(fs, "self._WriteBpString(_ss, self.%s[_i])", vardata->variable_name);
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

				// these variables has been process in bond_vars
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
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
	if (bond_vars == NULL) {
		// no variables, write 1 blank byte. because sizeof(empty_struct) == 1u
		BPCGEN_INDENT_PRINT;
		fputs("_ss.write(b'\\0')", fs);
	}
	// func deserialize is over
	BPCGEN_INDENT_DEC;

	// class define is over
	BPCGEN_INDENT_DEC;

	// free all cache data
	bpcgen_destructor_bond_vars(bond_vars);
}

static void write_opcode_enum(FILE* fs, GSList* msg_ls) {
	GSList* cursor;
	BPCGEN_INDENT_INIT_NEW(fs);

	// write opcode enum
	BPCGEN_INDENT_PRINT;
	fputs("class OpCode(enum.IntEnum):", fs); BPCGEN_INDENT_INC;
	for (cursor = msg_ls; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_MSG* data = (BPCSMTV_MSG*)cursor->data;

		BPCGEN_INDENT_PRINT;
		fprintf(fs, "%s: int = %" PRIu32, data->msg_name, data->msg_index);
	}
	if (msg_ls == NULL) {
		fputs("pass", fs);
	}
	// class opcode is over
	BPCGEN_INDENT_DEC;
}

static void write_msg_factory(FILE* fs, GSList* msg_ls) {
	GSList* cursor;
	BPCGEN_INDENT_INIT_NEW(fs);

	// write all msgs tuple first
	BPCGEN_INDENT_PRINT;
	fputs("_all_msgs: tuple[type] = (", fs); BPCGEN_INDENT_INC;
	for (cursor = msg_ls; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_MSG* data = (BPCSMTV_MSG*)cursor->data;

		// print tuple item
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "%s,", data->msg_name);
	}
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc(')', fs);

	// write uniformed deserialize func
	BPCGEN_INDENT_PRINT;
	fputs("def MessageFactory(_opcode: OpCode) -> BpMessage:", fs); BPCGEN_INDENT_INC;
	// check opcode overflow
	BPCGEN_INDENT_PRINT;
	fputs("if _opcode < 0 or _opcode >= len(_all_msgs): return None", fs);
	// run real constructor
	BPCGEN_INDENT_PRINT;
	fputs("else:", fs); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fputs("_data = _all_msgs[_opcode]()", fs);
	BPCGEN_INDENT_PRINT;
	fputs("return _data", fs);
	BPCGEN_INDENT_DEC;	// it's over
	// uniform func is over
	BPCGEN_INDENT_DEC;

}

void codepy_write_document(FILE* fs, BPCSMTV_DOCUMENT* document) {
	// write header
	bpcfs_write_snippets(fs, &bpcsnp_py_header);

	// write opcode
	GSList* msg_ls = bpcgen_constructor_msg_list(document->protocol_body);
	write_opcode_enum(fs, msg_ls);

	// write functions
	bpcfs_write_snippets(fs, &bpcsnp_py_functions);

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
				struct_like.is_msg = false;
				struct_like.real_ptr.pStruct = data->node_data.struct_data;
				write_struct_or_msg(fs, &struct_like);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG:
				struct_like.is_msg = true;
				struct_like.real_ptr.pMsg = data->node_data.msg_data;
				write_struct_or_msg(fs, &struct_like);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS:
				break;
			default:
				g_assert_not_reached();
		}
	}

	// write uniform deserialize
	write_msg_factory(fs, msg_ls);

	// free msg list
	bpcgen_destructor_msg_list(msg_ls);

}
