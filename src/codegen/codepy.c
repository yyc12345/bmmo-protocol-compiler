#include "../bpc_code_gen.h"

static const char python_struct_fmt[] = {
	'f', 'd', 'b', 'h', 'i', 'q', 'B', 'H', 'I', 'Q'
};
static const char python_struct_fmt_len[] = {
	'4', '8', '1', '2', '4', '8', '1', '2', '4', '8'
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

/// <summary>
/// 
/// </summary>
/// <param name="in_variables">variable list</param>
/// <param name="in_is_narrow"></param>
/// <param name="out_param_list"></param>
/// <param name="out_pack_fmt"></param>
/// <returns>the next node need manually process. if it equal with "in_variables", it mean that all output is invalid</returns>
static GSList* building_pack_param(GSList* in_variables, bool in_is_narrow, const char* in_prefix, GPtrArray* io_param_list, GString* io_pack_fmt) {
	GSList* cursor;
	GString* new_prefix = g_string_new(NULL);

	for (cursor = in_variables; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_VARIABLE* variable = (BPCSMTV_VARIABLE*)cursor->data;

		// check requirements
		if (variable->variable_array->is_array && !variable->variable_array->is_static_array) {
			// dynamic array is not allowed
			break;
		}
		if (variable->variable_type->full_uncover_is_basic_type) {
			// string is not allowed
			if (variable->variable_type->full_uncover_basic_type == BPCSMTV_BASIC_TYPE_STRING) {
				break;
			}
		} else {
			// struct
			if (in_is_narrow) {
				// we order user process struct manually in narrow mode,
				// because we cannot ensure its can be parsed into natural.
				break;
			}
			// however struct is not a problem in natural mode.
			// because we can expand it in limited format.
		}

		// generate data
		// use do-while to process array and non-array correctly
		uint32_t c = UINT32_C(0);
		do {
			// init prefix
			g_string_printf(new_prefix, "%s.%s", in_prefix, variable->variable_name);
			if (variable->variable_array->is_array) {
				g_string_append_printf(new_prefix, "[%" PRIu32 "]", c);
			}

			// getting format
			if (variable->variable_type->full_uncover_is_basic_type) {
				// basic types
				g_ptr_array_add(io_param_list, g_strdup(new_prefix->str));
				g_string_append_c(io_pack_fmt, python_struct_fmt[(size_t)variable->variable_type->full_uncover_basic_type]);
			} else {
				// struct types
				// recursively call this method with new prefix
				// get corresponding data first
				BPCSMTV_PROTOCOL_BODY* body = bpcsmtv_registery_identifier_get(variable->variable_type->type_data.custom_type);
				building_pack_param(body->node_data.struct_data->struct_body, in_is_narrow, new_prefix->str, io_param_list, io_pack_fmt);
			}

			// non-array immdiate breaker
			if (!variable->variable_array->is_array)
				break;

		} while (++c < variable->variable_array->static_array_len);

		// padding placeholder
		// we do not need consider natural/narrow here.
		// because in codegen, struct layout has degenerated and
		// only can describe code style. align is not related to layout. 
		// it has been processed properly by smtv functions.
		if (variable->variable_align->use_align) {
			g_string_append_printf(io_pack_fmt, "%" PRIu32 "x", variable->variable_align->padding_size);
		}

	}

	// free and return
	g_string_free(new_prefix, true);
	return cursor;
}

typedef struct _BOND_VARS {
	GSList* variables_node;
	bool is_manual_proc;
	struct {
		GPtrArray* param_list;
		GString* pack_fmt;
		uint32_t pystruct_index;
	}auto_proc_data;
}BOND_VARS;
static GPtrArray* constructor_bond_vars(GSList* variables, bool is_narrow) {
	GPtrArray* result = g_ptr_array_new_with_free_func(g_free);
	GSList* cursor = NULL, * old_cursor = NULL;
	uint32_t index_counter = UINT32_C(0);

	cursor = variables;
	if (variables == NULL) return result;
	do {
		// init fields
		BOND_VARS* data = g_new0(BOND_VARS, 1);
		data->variables_node = cursor;
		data->auto_proc_data.param_list = g_ptr_array_new_with_free_func(g_free);
		data->auto_proc_data.pack_fmt = g_string_new(NULL);

		// do parse
		old_cursor = cursor;
		cursor = building_pack_param(cursor, is_narrow, "self",
			data->auto_proc_data.param_list, data->auto_proc_data.pack_fmt);

		// manual proc and auto proc check
		data->is_manual_proc = old_cursor == cursor;
		if (data->is_manual_proc) {
			// manual proc need manually shift to next
			cursor = cursor->next;
		} else {
			data->auto_proc_data.pystruct_index = index_counter++;
		}
		
		// add array
		g_ptr_array_add(result, data);
	} while (cursor != NULL);

	// assert for natural
	if (!is_narrow)
		g_assert(result->len == 1u);

	return result;
}
static void destructor_bond_vars(GPtrArray* arr) {
	guint c;
	for (c = 0u; c < arr->len; ++c) {
		BOND_VARS* data = (BOND_VARS*)(arr->pdata[c]);
		// free internal data
		g_ptr_array_free(data->auto_proc_data.param_list, true);
		g_string_free(data->auto_proc_data.pack_fmt, true);
	}
	// free self
	g_ptr_array_free(arr, true);
}

static void write_enum(FILE* fs, BPCSMTV_ENUM* smtv_enum) {
	GSList* cursor;
	BPCGEN_INDENT_INIT_NEW(fs);

	BPCGEN_INDENT_PRINT;
	fprintf(fs, "class %s():", smtv_enum->enum_name); BPCGEN_INDENT_INC;
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

typedef union _SubstantialStructUnion {
	BPCSMTV_STRUCT* pStruct;
	BPCSMTV_MSG* pMsg;
}SubstantialStructUnion;
static void write_substantial_struct(FILE* fs, SubstantialStructUnion* union_data, bool is_msg) {
	GSList* cursor = NULL, * variables = (is_msg ? union_data->pMsg->msg_body : union_data->pStruct->struct_body);
	BPCSMTV_STRUCT_MODIFIER* modifier = (is_msg ? union_data->pMsg->msg_modifier : union_data->pStruct->struct_modifier);
	char* substantial_struct_name = (is_msg ? union_data->pMsg->msg_name : union_data->pStruct->struct_name);
	GString* oper = g_string_new(NULL);
	guint c = 0u;
	BPCGEN_INDENT_INIT_NEW(fs);

	// get bond variables, combine possible natural nodes
	GPtrArray* bond_var = constructor_bond_vars(variables, modifier->is_narrow);

	// class header
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "class %s():", substantial_struct_name); BPCGEN_INDENT_INC;

	// static natural struct pack
	BPCGEN_INDENT_PRINT;
	fputs("_struct_packer = (", fs);
	for (c = 0u; c < bond_var->len; ++c) {
		BOND_VARS* data = (BOND_VARS*)(bond_var->pdata[c]);
		if (!data->is_manual_proc) {
			fprintf(fs, "struct.Struct('<%s'), ", data->auto_proc_data.pack_fmt->str);
		}
	}
	fputs(")", fs);

	// class constructor
	BPCGEN_INDENT_PRINT;
	fputs("def __init__(self):", fs); BPCGEN_INDENT_INC;
	for (cursor = variables; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_VARIABLE* data = (BPCSMTV_VARIABLE*)cursor->data;

		// array loop header and determin oper name
		if (data->variable_array->is_array) {
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "self.%s = []", data->variable_name);

			if (data->variable_array->is_static_array) {
				BPCGEN_INDENT_PRINT;
				fprintf(fs, "for _i in range(%" PRIu32 "):", data->variable_array->static_array_len); BPCGEN_INDENT_INC;
			} else {
				// pass for dynamic list, so we provide a fake for
				BPCGEN_INDENT_PRINT;
				fputs("for _i in range(0):", fs); BPCGEN_INDENT_INC;
			}

			g_string_assign(oper, "_cache");
		} else {
			g_string_printf(oper, "self.%s", data->variable_name);
		}

		// write body
		BPCGEN_INDENT_PRINT;
		if (data->variable_type->full_uncover_is_basic_type) {
			fprintf(fs, "%s = None", oper->str);
		} else {
			fprintf(fs, "%s = %s()", oper->str, data->variable_type->type_data.custom_type);
		}

		// array tail
		if (data->variable_array->is_array) {
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "self.%s.append(_cache)", data->variable_name);
			BPCGEN_INDENT_DEC;
		}

	}
	if (variables == NULL) {
		// if there are no member, wee need write extra `pass`
		BPCGEN_INDENT_PRINT;
		fputs("pass", fs);
	}
	BPCGEN_INDENT_DEC;

	// reliablility getter and opcode getter for msg
	if (is_msg) {
		// reliable
		BPCGEN_INDENT_PRINT;
		fputs("def GetReliable(self) -> bool:", fs); BPCGEN_INDENT_INC;

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
	BPCGEN_INDENT_PRINT;
	fputs("def Deserialize(self, _ss: io.BytesIO):", fs); BPCGEN_INDENT_INC;
	if (is_msg) {
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "if _opcode_packer.unpack(_ss.read(_opcode_packer.size))[0] != %" PRIu32 ":", union_data->pMsg->msg_index); BPCGEN_INDENT_INC;
		BPCGEN_INDENT_PRINT;
		fputs("raise Exception('Invalid opcode!')", fs);
		BPCGEN_INDENT_DEC;
	}
	for (c = 0u; c < bond_var->len; ++c) {
		BOND_VARS* bdata = (BOND_VARS*)(bond_var->pdata[c]);

		if (bdata->is_manual_proc) {
			BPCSMTV_VARIABLE* data = (BPCSMTV_VARIABLE*)(bdata->variables_node->data);

			// annotation
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "# %s", data->variable_name);

			// if member is array, we need construct a loop
			// and determine oper name
			if (data->variable_array->is_array) {
				if (data->variable_array->is_static_array) {
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for _i in range(%d):", data->variable_array->static_array_len);

					g_string_printf(oper, "self.%s[_i]", data->variable_name);
				} else {
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s.clear()", data->variable_name);
					BPCGEN_INDENT_PRINT;
					fputs("_count = _listlen_packer.unpack(_ss.read(_listlen_packer.size))[0]", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for _i in range(_count):");

					g_string_assign(oper, "_cache");
				}

				BPCGEN_INDENT_INC;
			} else {
				g_string_printf(oper, "self.%s", data->variable_name);
			}

			// resolve its type to determine proper read method
			if (data->variable_type->full_uncover_is_basic_type) {
				// native deserialize
				// use different strategy for string and other types.
				BPCGEN_INDENT_PRINT;
				if (data->variable_type->full_uncover_basic_type == BPCSMTV_BASIC_TYPE_STRING) {
					fprintf(fs, "%s = _read_bp_string(_ss)", oper->str);
				} else {
					fprintf(fs, "%s = struct.unpack('<%c', _ss.read(%c))[0]",
						oper->str,
						python_struct_fmt[(size_t)data->variable_type->full_uncover_basic_type],
						python_struct_fmt_len[(size_t)data->variable_type->full_uncover_basic_type]);
				}
			} else {
				// call struct.deserialize()
				BPCGEN_INDENT_PRINT;
				fprintf(fs, "%s = %s()", oper->str, data->variable_type->type_data.custom_type);
				BPCGEN_INDENT_PRINT;
				fprintf(fs, "%s.Deserialize(_ss)", oper->str);
			}

			// array extra proc
			if (data->variable_array->is_array) {
				// only dynamic array need append
				if (!data->variable_array->is_static_array) {
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "self.%s.append(_cache)", data->variable_name);
				}

				BPCGEN_INDENT_DEC;
			}

			// compute align data
			if (data->variable_align->use_align) {
				BPCGEN_INDENT_PRINT;
				fprintf(fs, "_ss.read(%" PRIu32 ")", data->variable_align->padding_size);
			}

		} else {
			// annotation
			BPCGEN_INDENT_PRINT;
			fputs("# ", fs);
			write_args_series(fs, bdata->auto_proc_data.param_list);

			// binary reader
			BPCGEN_INDENT_PRINT;
			fputc('(', fs);
			write_tuple_series(fs, bdata->auto_proc_data.param_list);
			fprintf(fs, ") = _struct_packer[%" PRIu32 "].unpack(_ss.read(_struct_packer[%" PRIu32 "].size))",
				bdata->auto_proc_data.pystruct_index, bdata->auto_proc_data.pystruct_index);

		}
	}
	if (bond_var->len == 0u && (!is_msg)) {
		// no variable in struct, write pass
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "pass");
	}
	BPCGEN_INDENT_DEC;

	// serialize func
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "def Serialize(self, _ss: io.BytesIO):"); BPCGEN_INDENT_INC;
	if (is_msg) {
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "_ss.write(_opcode_packer.pack(%" PRIu32 "))", union_data->pMsg->msg_index);
	}
	for (c = 0u; c < bond_var->len; ++c) {
		BOND_VARS* bdata = (BOND_VARS*)(bond_var->pdata[c]);

		if (bdata->is_manual_proc) {
			BPCSMTV_VARIABLE* data = (BPCSMTV_VARIABLE*)(bdata->variables_node->data);

			// annotation
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "# %s", data->variable_name);

			// construct array loop and determine oper name
			if (data->variable_array->is_array) {
				if (data->variable_array->is_static_array) {
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for _i in range(%" PRIu32 "):", data->variable_array->static_array_len); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_item = self.%s[_i]", data->variable_name);
				} else {
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_ss.write(_listlen_packer.pack(len(self.%s)))", data->variable_name);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for _item in self.%s:", data->variable_name); BPCGEN_INDENT_INC;
				}

				g_string_assign(oper, "_item");
			} else {
				g_string_printf(oper, "self.%s", data->variable_name);
			}

			// determine proper write method
			if (data->variable_type->full_uncover_is_basic_type) {
				// native serializer
				BPCGEN_INDENT_PRINT;
				if (data->variable_type->full_uncover_basic_type == BPCSMTV_BASIC_TYPE_STRING) {
					fprintf(fs, "_write_bp_string(_ss, %s)", oper->str);
				} else {
					fprintf(fs, "_ss.write(struct.pack('%c', %s))",
						python_struct_fmt[(size_t)data->variable_type->full_uncover_basic_type],
						oper->str);
				}
			} else {
				// struct serialize
				BPCGEN_INDENT_PRINT;
				fprintf(fs, "%s.Serialize(_ss)", oper->str);
			}

			// array tail
			if (data->variable_array->is_array) {
				// shink indent
				BPCGEN_INDENT_DEC;
			}

			// align padding
			if (data->variable_align->use_align) {
				BPCGEN_INDENT_PRINT;
				fprintf(fs, "_ss.write(b'\\0' * %" PRIu32 ")", data->variable_align->padding_size);
			}

		} else {
			// annotation
			BPCGEN_INDENT_PRINT;
			fputs("# ", fs);
			write_args_series(fs, bdata->auto_proc_data.param_list);

			// binary writer
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "_ss.write(_struct_packer[%" PRIu32 "].pack(", bdata->auto_proc_data.pystruct_index);
			write_args_series(fs, bdata->auto_proc_data.param_list);
			fputs("))", fs);
		}

	}
	if (bond_var == 0u && (!is_msg)) {
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
	destructor_bond_vars(bond_var);
}

static void write_opcode_enum(FILE* fs, GSList* msg_ls) {
	GSList* cursor;
	BPCGEN_INDENT_INIT_NEW(fs);

	// write opcode enum
	BPCGEN_INDENT_PRINT;
	fputs("class opcode():", fs); BPCGEN_INDENT_INC;
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
	fputs("def _UniformDeserialize(_ss: io.BytesIO):", fs); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fputs("_opcode = _opcode_packer.unpack(_ss.read(_opcode_packer.size))[0]", fs);
	for (cursor = msg_ls; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_MSG* data = (BPCSMTV_MSG*)cursor->data;

		BPCGEN_INDENT_PRINT;
		// print if or elif statements
		if (is_first) is_first = false;
		else fputs("el", fs);
		fprintf(fs, "if _opcode == opcode.%s:", data->msg_name);

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
	GSList* cursor = NULL, * msg_ls = NULL;
	SubstantialStructUnion substantial_struct;
	for (cursor = document->protocol_body; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_PROTOCOL_BODY* data = (BPCSMTV_PROTOCOL_BODY*)cursor->data;

		switch (data->node_type) {
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM:
				write_enum(fs, data->node_data.enum_data);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT:
				substantial_struct.pStruct = data->node_data.struct_data;
				write_substantial_struct(fs, &substantial_struct, false);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG:
				substantial_struct.pMsg = data->node_data.msg_data;
				write_substantial_struct(fs, &substantial_struct, true);

				msg_ls = g_slist_append(msg_ls, data->node_data.msg_data);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS:
			default:
				// do nothing for alias and invalid type
				break;
		}
	}

	// write some tails
	write_opcode_enum(fs, msg_ls);
	write_uniform_deserialize(fs, msg_ls);

	// free msg list
	if (msg_ls != NULL)
		g_slist_free(msg_ls);

}
