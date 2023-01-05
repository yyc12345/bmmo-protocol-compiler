#include "../bpc_code_gen.h"

static char* get_primitive_type_name(BPCSMTV_VARIABLE* variable, const char* parent_name) {
	if (variable->variable_type->semi_uncover_is_basic_type) {
		switch (variable->variable_type->full_uncover_basic_type) {
			case BPCSMTV_BASIC_TYPE_FLOAT:
				return "float";
			case BPCSMTV_BASIC_TYPE_DOUBLE:
				return "double";

			case BPCSMTV_BASIC_TYPE_INT8:
			case BPCSMTV_BASIC_TYPE_INT16:
				bpcerr_warning(BPCERR_ERROR_SOURCE_CODEGEN, "For msg/struct %s, variable %s, Protobuf do not support INT8 and INT16, so compiler upgrade them to INT32.",
					parent_name, variable->variable_name);
			case BPCSMTV_BASIC_TYPE_INT32:
				return "int32";
			case BPCSMTV_BASIC_TYPE_INT64:
				return "int64";

			case BPCSMTV_BASIC_TYPE_UINT8:
			case BPCSMTV_BASIC_TYPE_UINT16:
				bpcerr_warning(BPCERR_ERROR_SOURCE_CODEGEN, "For msg/struct %s, variable %s, Protobuf do not support UINT8 and UINT16, so compiler upgrade them to UINT32.",
					parent_name, variable->variable_name);
			case BPCSMTV_BASIC_TYPE_UINT32:
				return "uint32";
			case BPCSMTV_BASIC_TYPE_UINT64:
				return "uint64";

			case BPCSMTV_BASIC_TYPE_STRING:
				return "string";
			default:
				g_assert_not_reached();
		}
	} else {
		return variable->variable_type->semi_uncover_custom_type;
	}
}

static void write_enum(FILE* fs, BPCSMTV_ENUM* smtv_enum) {
	BPCGEN_INDENT_INIT_NEW(fs);
	GSList* cursor;

	BPCGEN_INDENT_PRINT;
	fprintf(fs, "enum %s {", smtv_enum->enum_name); BPCGEN_INDENT_INC;
	bpcerr_warning(BPCERR_ERROR_SOURCE_CODEGEN, "For enum %s, Protobuf do not support the implicit type of enum.", smtv_enum->enum_name);
	for (cursor = smtv_enum->enum_body; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_ENUM_MEMBER* data = (BPCSMTV_ENUM_MEMBER*)cursor->data;

		// check first node
		if (cursor == smtv_enum->enum_body) {
			if ((data->distributed_value_is_uint && data->distributed_value.value_uint != UINT64_C(0)) ||
				(!data->distributed_value_is_uint && data->distributed_value.value_int != INT64_C(0))) {
				bpcerr_warning(BPCERR_ERROR_SOURCE_CODEGEN, "For enum %s, Protobuf do not allow non-zero value for the first member.", smtv_enum->enum_name);
			}
		}

		BPCGEN_INDENT_PRINT;
		if (data->distributed_value_is_uint) {
			fprintf(fs, "%s = %" PRIu64 ";", data->enum_member_name, data->distributed_value.value_uint);
		} else {
			fprintf(fs, "%s = %" PRIi64 ";", data->enum_member_name, data->distributed_value.value_int);
		}
	}
	// enum is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

}

static void write_struct_or_msg(FILE* fs, BPCGEN_STRUCT_LIKE* union_data, bool is_msg) {
	GSList* cursor = NULL, * variables = (is_msg ? union_data->pMsg->msg_body : union_data->pStruct->struct_body);
	BPCSMTV_STRUCT_MODIFIER* modifier = (is_msg ? union_data->pMsg->msg_modifier : union_data->pStruct->struct_modifier);
	char* struct_like_name = (is_msg ? union_data->pMsg->msg_name : union_data->pStruct->struct_name);
	uint32_t index = UINT32_C(1);
	BPCGEN_INDENT_INIT_NEW(fs);

	// we only need iterate all variables, so we do not group them
	GSList* bond_vars = bpcgen_constructor_bond_vars(variables, BPCGEN_VARTYPE_NONE);

	// info for struct convertion
	if (!is_msg) {
		bpcerr_info(BPCERR_ERROR_SOURCE_CODEGEN, "Struct %s has been migrated as message.", struct_like_name);
	}

	// message body
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "message %s {", struct_like_name); BPCGEN_INDENT_INC;
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
					fprintf(fs, "%s %s = %" PRIu32 ";", get_primitive_type_name(vardata, struct_like_name), vardata->variable_name, index++);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					bpcerr_warning(BPCERR_ERROR_SOURCE_CODEGEN,
						"For msg/struct %s, variable %s, Protobuf do not support static array. We use \"repeated\" instead.",
						struct_like_name, vardata->variable_name
					);
					// break;	// disable break to go through by design.
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "repeated %s %s = %" PRIu32 ";", get_primitive_type_name(vardata, struct_like_name), vardata->variable_name, index++);
					break;
				}

				default:
					g_assert_not_reached();
			}

			// padding warning
			if (vardata->variable_align->use_align) {
				bpcerr_warning(BPCERR_ERROR_SOURCE_CODEGEN,
					"For msg/struct %s, variable %s, Protobuf do not support align.",
					struct_like_name, vardata->variable_name
				);
			}
		}
	}
	// message is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

}

static char* generate_namespaces(GSList* ns_list) {
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

void codeproto_write_document(FILE* fs, BPCSMTV_DOCUMENT* document) {
	BPCGEN_INDENT_INIT_NEW(fs);

	// write syntax
	fputs("syntax = \"proto3\";", fs);

	// write package
	char* ns = generate_namespaces(document->namespace_data);
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "package %s;", ns);
	g_free(ns);

	// iterate list to get data
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
				break;	// protobuf do not support alias
			default:
				g_assert_not_reached();
		}
	}

}
