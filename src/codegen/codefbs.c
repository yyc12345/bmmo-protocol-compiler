#include "../bpc_code_gen.h"

static const char* fbs_basic_type[] = {
	"float", "double", "byte", "short", "int", "long", "ubyte", "ushort", "uint", "ulong", "string"
};
static const char* get_primitive_type_name(BPCSMTV_VARIABLE* variable) {
	return (variable->variable_type->semi_uncover_is_basic_type?
		fbs_basic_type[variable->variable_type->full_uncover_basic_type]:
		(const char*)variable->variable_type->semi_uncover_custom_type
	);
}

static void write_enum(FILE* fs, BPCSMTV_ENUM* smtv_enum) {
	BPCGEN_INDENT_INIT_NEW(fs);
	GSList* cursor;

	BPCGEN_INDENT_PRINT;
	fprintf(fs, "enum %s : %s {", smtv_enum->enum_name, fbs_basic_type[smtv_enum->enum_basic_type]); BPCGEN_INDENT_INC;
	for (cursor = smtv_enum->enum_body; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_ENUM_MEMBER* data = (BPCSMTV_ENUM_MEMBER*)cursor->data;

		// print node
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "%s = ", data->enum_member_name);
		bpcgen_print_enum_member_value(fs, data);

		if (cursor->next != NULL) {
			fputc(',', fs);
		}
	}
	// enum is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

}

static bool has_static_array(GSList* bond_vars) {
	GSList* cursor = NULL;
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			switch (data->vars_type[c]) {
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					return true;	// has static array
				}
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_STRING:
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					break;
				}

				default:
					g_assert_not_reached();
			}
		}
	}

	return false;	// do not have static array
}

static void write_struct_or_msg(FILE* fs, BPCGEN_STRUCT_LIKE* union_data) {
	GSList* cursor = NULL;
	bool is_msg;  GSList* variables; char* struct_like_name;
	bpcgen_pick_struct_like_data(union_data, &is_msg, &variables, NULL, &struct_like_name);
	BPCGEN_INDENT_INIT_NEW(fs);

	// we only need iterate all variables, so we do not group them
	GSList* bond_vars = bpcgen_constructor_bond_vars(variables, BPCGEN_VARTYPE_NONE);

	// message body
	// decide table or struct
	BPCGEN_INDENT_PRINT;
	if (is_msg) {
		if (has_static_array(bond_vars)) {
			// downgrade to struct
			bpcerr_codegen_warning(BPCERR_ERROR_CODEGEN_SOURCE_FLATBUFFERS,
				"Downgrade \"%s\" from table to struct because flatbuffers do not support array in table.",
				struct_like_name
			);
			fputs("struct", fs);
		} else {
			// still use table
			fputs("table", fs);
		}
	} else {
		// use struct in default
		fputs("struct", fs);
	}
	// write name and bracket
	fprintf(fs, " %s {", struct_like_name); BPCGEN_INDENT_INC;

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
					fprintf(fs, "%s:%s;", vardata->variable_name, get_primitive_type_name(vardata));
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s:[%s:%" PRIu32 "];", vardata->variable_name, get_primitive_type_name(vardata), vardata->variable_array->static_array_len);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s:[%s];", vardata->variable_name, get_primitive_type_name(vardata));
					break;
				}

				default:
					g_assert_not_reached();
			}

		}
	}
	// message is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// free all cache data
	bpcgen_destructor_bond_vars(bond_vars);

}

void codefbs_write_document(FILE* fs, BPCSMTV_DOCUMENT* document) {
	BPCGEN_INDENT_INIT_NEW(fs);
	
	// raise general warning for fbs
	bpcerr_codegen_warning(BPCERR_ERROR_CODEGEN_SOURCE_FLATBUFFERS,
		"Flatbuffers output is designed for migration. Some properties may be ignored in migration, such as padding and align."
	);

	// write namespace
	BPCGEN_INDENT_PRINT;
	fputs("namespace ", fs);
	bpcgen_print_join_gslist(fs, ".", true, document->namespace_data);
	fputc(';', fs);

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
				break;	// flatbuffers do not support alias
			default:
				g_assert_not_reached();
		}
	}

}
