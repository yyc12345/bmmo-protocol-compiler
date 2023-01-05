#include "bpc_code_gen.h"
#include <stdio.h>
#include <inttypes.h>
#include "bpc_error.h"
#include "bpc_fs.h"

#pragma region Basic function

static FILE* fs_python = NULL;
static FILE* fs_csharp = NULL;
static FILE* fs_cpp_hdr = NULL;
static FILE* fs_cpp_src = NULL;
static FILE* fs_proto = NULL;
static gchar* hpp_reference = NULL;

void bpcgen_init_code_file(BPCCMD_PARSED_ARGS* bpc_args) {
	fs_python = bpc_args->out_python_file;
	fs_csharp = bpc_args->out_csharp_file;
	fs_cpp_hdr = bpc_args->out_cpp_header_file;
	fs_cpp_src = bpc_args->out_cpp_source_file;
	fs_proto = bpc_args->out_proto_file;

	hpp_reference = g_strdup(bpc_args->ref_cpp_relative_hdr);
}

void bpcgen_write_document(BPCSMTV_DOCUMENT* document) {
	if (fs_python != NULL) codepy_write_document(fs_python, document);
	if (fs_csharp != NULL) codecs_write_document(fs_csharp, document);
	if (fs_cpp_hdr != NULL) codehpp_write_document(fs_cpp_hdr, document);
	if (fs_cpp_src != NULL) codecpp_write_document(fs_cpp_src, document, hpp_reference);
	if (fs_proto != NULL) codeproto_write_document(fs_proto, document);
}

void bpcgen_free_code_file() {
	// just empty file stream variables in this file.
	// the work of file free is taken by commandline parser.
	fs_python = fs_csharp = fs_cpp_hdr = fs_cpp_src = fs_proto = NULL;
	g_free(hpp_reference);
}

#pragma endregion

#pragma region Bond vars

/// <summary>
/// detect BPCGEN_VARTYPE from specific variable
/// </summary>
/// <param name="variable"></param>
/// <returns></returns>
static BPCGEN_VARTYPE get_vartype(BPCSMTV_VARIABLE* variable) {
	if (variable->variable_type->full_uncover_is_basic_type) {
		// distinguish string and primitive types
		if (variable->variable_type->full_uncover_basic_type == BPCSMTV_BASIC_TYPE_STRING) {

			if (variable->variable_array->is_array) {
				if (variable->variable_array->is_static_array) return BPCGEN_VARTYPE_STATIC_STRING;
				else return BPCGEN_VARTYPE_DYNAMIC_STRING;
			} else return BPCGEN_VARTYPE_SINGLE_STRING;

		} else {

			if (variable->variable_array->is_array) {
				if (variable->variable_array->is_static_array) return BPCGEN_VARTYPE_STATIC_PRIMITIVE;
				else return BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE;
			} else return BPCGEN_VARTYPE_SINGLE_PRIMITIVE;

		}
	} else {
		// distinguish narrow and natural struct
		BPCSMTV_PROTOCOL_BODY* ref_struct = bpcsmtv_registery_identifier_get(variable->variable_type->type_data.custom_type);
		g_assert(ref_struct->node_type == BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT);

		if (ref_struct->node_data.struct_data->struct_modifier->is_narrow) {

			if (variable->variable_array->is_array) {
				if (variable->variable_array->is_static_array) return BPCGEN_VARTYPE_STATIC_NARROW;
				else return BPCGEN_VARTYPE_DYNAMIC_NARROW;
			} else return BPCGEN_VARTYPE_SINGLE_NARROW;

		} else {

			if (variable->variable_array->is_array) {
				if (variable->variable_array->is_static_array) return BPCGEN_VARTYPE_STATIC_NATURAL;
				else return BPCGEN_VARTYPE_DYNAMIC_NATURAL;
			} else return BPCGEN_VARTYPE_SINGLE_NATURAL;

		}
	}
}

/// <summary>
/// create bond vars recursively. internally use only.
/// </summary>
/// <param name="in_variables"></param>
/// <param name="in_bond_rules"></param>
/// <param name="out_bond_vars">a pointer to "BOND_VARS*". set NULL if no data returned. this variables is allocated by this function and should be free by caller.</param>
/// <returns></returns>
static GSList* building_bond_vars(GSList* in_variables, BPCGEN_VARTYPE in_bond_rules, BOND_VARS** out_bond_vars) {
	// preset
	*out_bond_vars = NULL;
	if (in_variables == NULL) return NULL;

	// loop data until facing requirements
	GSList* cursor;
	uint32_t counter = UINT32_C(0);
	for (cursor = in_variables; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_VARIABLE* variable = (BPCSMTV_VARIABLE*)cursor->data;

		// get current variable type
		BPCGEN_VARTYPE current_var_type = get_vartype(variable);

		// compare with rules
		if (!BPCGEN_VARTYPE_CONTAIN(in_bond_rules, current_var_type)) {
			// stop. this variable is not matched
			break;
		} else {
			// goto next one
			++counter;
		}
	}

	// create one
	BOND_VARS* bond_vars_oper = g_new0(BOND_VARS, 1);
	*out_bond_vars = bond_vars_oper;
	if (counter == UINT32_C(0)) {
		// nothing matched
		// init bond vars
		bond_vars_oper->is_bonded = false;
		bond_vars_oper->bond_vars_len = 1;	// set 1 for convenient loop even in not bounded vars
		bond_vars_oper->plist_vars = g_new0(BPCSMTV_VARIABLE*, 1);
		bond_vars_oper->vars_type = g_new0(BPCGEN_VARTYPE, 1);
		// set values
		bond_vars_oper->plist_vars[0] = (BPCSMTV_VARIABLE*)in_variables->data;
		bond_vars_oper->vars_type[0] = get_vartype((BPCSMTV_VARIABLE*)in_variables->data);
		// return next
		return in_variables->next;
	} else {
		// matched, is bond_vars
		// init bond vars
		bond_vars_oper->is_bonded = true;
		bond_vars_oper->bond_vars_len = counter;
		bond_vars_oper->plist_vars = g_new0(BPCSMTV_VARIABLE*, counter);
		bond_vars_oper->vars_type = g_new0(BPCGEN_VARTYPE, counter);
		// set values
		uint32_t p; cursor = in_variables;
		for (p = 0; p < counter; ++p) {
			bond_vars_oper->plist_vars[p] = (BPCSMTV_VARIABLE*)cursor->data;
			bond_vars_oper->vars_type[p] = get_vartype((BPCSMTV_VARIABLE*)cursor->data);
			cursor = cursor->next;
		}
		// return next
		return cursor;
	}

}

GSList* bpcgen_constructor_bond_vars(GSList* variables, BPCGEN_VARTYPE bond_rules) {
	GSList* result = NULL;
	BOND_VARS* cache_bond_vars = NULL;

	GSList* cursor = variables;
	while (true) {
		cursor = building_bond_vars(cursor, bond_rules, &cache_bond_vars);
		if (cache_bond_vars == NULL) break;

		result = g_slist_append(result, cache_bond_vars);
	}

	return result;
}

void bpcgen_destructor_bond_vars(GSList* bond_vars) {
	if (bond_vars == NULL) return;

	// free every items
	GSList* cursor;
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* item = (BOND_VARS*)cursor->data;

		// free sub item
		g_free(item->plist_vars);
		g_free(item->vars_type);
		// free item self
		g_free(item);
	}

	// free self
	g_slist_free(bond_vars);
}

#pragma endregion

#pragma region msg list

GSList* bpcgen_constructor_msg_list(GSList* protocol_body) {
	GSList* cursor = NULL, * msg_ls = NULL;
	for (cursor = protocol_body; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_PROTOCOL_BODY* data = (BPCSMTV_PROTOCOL_BODY*)cursor->data;

		switch (data->node_type) {
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG:
				msg_ls = g_slist_append(msg_ls, data->node_data.msg_data);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS:
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM:
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT:
			default:
				break;
		}
	}

	return msg_ls;
}
void bpcgen_destructor_msg_list(GSList* msg_ls) {
	if (msg_ls == NULL) return;
	g_slist_free(msg_ls);
}

#pragma endregion

#pragma region convenient functions

void bpcgen_print_enum_member(FILE* fs, BPCSMTV_ENUM_MEMBER* data) {
	if (data->distributed_value_is_uint) {
		fprintf(fs, "%s = %" PRIu64, data->enum_member_name, data->distributed_value.value_uint);
	} else {
		fprintf(fs, "%s = %" PRIi64, data->enum_member_name, data->distributed_value.value_int);
	}
}

void bpcgen_print_join_ptrarray(FILE* fs, const char* splitter, bool no_tail, GPtrArray* parr) {
	guint c;
	for (c = 0u; c < parr->len; ++c) {
		// attach splittor
		if (c != 0u && splitter != NULL) {
			fputs(splitter, fs);
		}

		fputs((gchar*)(parr->pdata[c]), fs);
	}

	// extra tail
	if (!no_tail && splitter != NULL) {
		fputs(splitter, fs);
	}
}

void bpcgen_print_join_gslist(FILE* fs, const char* splitter, bool no_tail, GSList* pslist) {
	GSList* cursor = NULL;

	for (cursor = pslist; cursor != NULL; cursor = cursor->next) {
		if (cursor != pslist && splitter != NULL) {
			fputs(splitter, fs);
		}
		
		fputs((gchar*)cursor->data, fs);
	}

	// extra tail
	if (!no_tail && splitter != NULL) {
		fputs(splitter, fs);
	}
}

void bpcgen_print_variables_annotation(FILE* fs, const char* annotation, BOND_VARS* data) {
	uint32_t c;

	// annotation header
	if (annotation != NULL) fputs(annotation, fs);
	for (c = 0; c < data->bond_vars_len; ++c) {
		BPCSMTV_VARIABLE* vardata = data->plist_vars[c];
		if (c != 0u) {
			fputs(", ", fs);
		}
		// variable name
		fputs(vardata->variable_name, fs);
	}
}

void bpcgen_pick_struct_like_data(BPCGEN_STRUCT_LIKE* union_data, bool* is_msg, GSList** variables, BPCSMTV_STRUCT_MODIFIER** modifier, char** struct_like_name) {
	if (is_msg != NULL) {
		*is_msg = union_data->is_msg;
	}
	if (variables != NULL) {
		*variables = (union_data->is_msg ? union_data->real_ptr.pMsg->msg_body : union_data->real_ptr.pStruct->struct_body);
	}
	if (modifier != NULL) {
		*modifier = (union_data->is_msg ? union_data->real_ptr.pMsg->msg_modifier : union_data->real_ptr.pStruct->struct_modifier);
	}
	if (struct_like_name != NULL) {
		*struct_like_name = (union_data->is_msg ? union_data->real_ptr.pMsg->msg_name : union_data->real_ptr.pStruct->struct_name);
	}
}

#pragma endregion

