#include "../bpc_code_gen.h"

static const char* cpp_basic_type[] = {
	"float", "double", "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "std::string"
};

static const char* get_primitive_type_name(BPCSMTV_VARIABLE* variable) {
	return (variable->variable_type->is_basic_type ?
		cpp_basic_type[variable->variable_type->type_data.basic_type] :
		(const char*)variable->variable_type->type_data.custom_type);
}

static void write_alias(FILE* fs, BPCSMTV_ALIAS* smtv_alias, BPCGEN_INDENT_TYPE indent) {
	BPCGEN_INDENT_INIT_REF(fs, indent);

	BPCGEN_INDENT_PRINT;
	fprintf(fs, "typedef %s %s;", cpp_basic_type[smtv_alias->basic_type], smtv_alias->custom_type);
}

static void write_enum(FILE* fs, BPCSMTV_ENUM* smtv_enum, BPCGEN_INDENT_TYPE indent) {
	BPCGEN_INDENT_INIT_REF(fs, indent);
	GSList* cursor;

	BPCGEN_INDENT_PRINT;
	fprintf(fs, "enum class %s : %s {", smtv_enum->enum_name, cpp_basic_type[smtv_enum->enum_basic_type]); BPCGEN_INDENT_INC;
	for (cursor = smtv_enum->enum_body; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_ENUM_MEMBER* data = (BPCSMTV_ENUM_MEMBER*)cursor->data;

		BPCGEN_INDENT_PRINT;
		bpcgen_print_enum_member(fs, data);

		if (cursor->next != NULL) {
			fputc(',', fs);
		}
	}
	// enum is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputs("};", fs);
}

static void write_struct_or_msg(FILE* fs, BPCGEN_STRUCT_LIKE* union_data, BPCGEN_INDENT_TYPE indent) {
	GSList* cursor = NULL;
	bool is_msg;  GSList* variables; BPCSMTV_STRUCT_MODIFIER* modifier; char* struct_like_name;
	bpcgen_pick_struct_like_data(union_data, &is_msg, &variables, &modifier, &struct_like_name);
	BPCGEN_INDENT_INIT_REF(fs, indent);

	// in hpp file, we do not need bond any variables. cpp will do it.
	GSList* bond_vars = bpcgen_constructor_bond_vars(variables, BPCGEN_VARTYPE_NONE);

	// class header
	BPCGEN_INDENT_PRINT;
	if (is_msg) {
		fprintf(fs, "class %s : public _BpMessage {", struct_like_name);
	} else {
		fprintf(fs, "class %s : public _BpStruct {", struct_like_name);
	}
	BPCGEN_INDENT_PRINT;
	fputs("public:", fs); BPCGEN_INDENT_INC;

	// internal data define
	// pack specific
	BPCGEN_INDENT_PRINT;
	if (modifier->is_narrow) {
		fputs("#pragma pack(1)", fs);	// narrow is always 1 byte align
	} else {
		fprintf(fs, "#pragma pack(%" PRIu32 ")", modifier->struct_unit_size);	// use calculated align size
	}
	BPCGEN_INDENT_PRINT;
	fputs("typedef struct {", fs); BPCGEN_INDENT_INC;
	uint32_t ph_counter = UINT32_C(0);
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			BPCSMTV_VARIABLE* vardata = data->plist_vars[c];
			switch (data->vars_type[c]) {
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s %s;", get_primitive_type_name(vardata), vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s::_InternalDataType %s;", get_primitive_type_name(vardata), vardata->variable_name);
					break;
				}

				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s %s[%" PRIu32 "];",
						get_primitive_type_name(vardata),
						vardata->variable_name,
						vardata->variable_array->static_array_len);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s::_InternalDataType %s[%" PRIu32 "];", 
						get_primitive_type_name(vardata), 
						vardata->variable_name, 
						vardata->variable_array->static_array_len);
					break;
				}

				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "std::vector<%s> %s;", get_primitive_type_name(vardata), vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "std::vector<%s*> %s;", get_primitive_type_name(vardata), vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "std::vector<%s::_InternalDataType*> %s;", get_primitive_type_name(vardata), vardata->variable_name);
					break;
				}

				default:
					g_assert_not_reached();
			}

			// padding, only applied in narrow struct.
			// because align always reflect variable align even in natural mode.
			// (for the convenience other languages, python and etc)
			if (modifier->is_narrow && vardata->variable_align->use_align) {
				BPCGEN_INDENT_PRINT;
				fprintf(fs, "char _placeholder%" PRIu32 "[%" PRIu32 "];", ph_counter++, vardata->variable_align->padding_size);
			}
		}
	}
	// internal data define is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputs("}_InternalDataType;", fs);
	// restore pack
	BPCGEN_INDENT_PRINT;
	fputs("#pragma pack()", fs);

	// declare internal data
	BPCGEN_INDENT_PRINT;
	fputs("_InternalDataType _InternalData;", fs);

	// declare common functions
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s();", struct_like_name);
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "virtual ~%s();", struct_like_name);
	BPCGEN_INDENT_PRINT;
	fputs("virtual bool Serialize(std::stringstream* _ss) override;", fs);
	BPCGEN_INDENT_PRINT;
	fputs("virtual bool Deserialize(std::stringstream* _ss) override;", fs);
	BPCGEN_INDENT_PRINT;
	fputs("static void _InnerConstructor(_InternalDataType* _p);", fs);
	BPCGEN_INDENT_PRINT;
	fputs("static void _InnerDestructor(_InternalDataType* _p);", fs);
	BPCGEN_INDENT_PRINT;
	fputs("static void _InnerSwap(_InternalDataType* _p);", fs);
	BPCGEN_INDENT_PRINT;
	fputs("static bool _InnerSerialize(_InternalDataType* _p, std::stringstream* _ss);", fs);
	BPCGEN_INDENT_PRINT;
	fputs("static bool _InnerDeserialize(_InternalDataType* _p, std::stringstream* _ss);", fs);

	// msg unique functions
	if (is_msg) {
		BPCGEN_INDENT_PRINT;
		fputs("virtual _OpCode GetOpCode() override;", fs);
		BPCGEN_INDENT_PRINT;
		fputs("virtual bool GetIsReliable() override;", fs);
	}

	// class is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputs("};", fs);

	// free all cache data
	bpcgen_destructor_bond_vars(bond_vars);
}

static void write_opcode_enum(FILE* fs, GSList* msg_ls, BPCGEN_INDENT_TYPE indent) {
	GSList* cursor;
	BPCGEN_INDENT_INIT_REF(fs, indent);

	// write opcode enum
	BPCGEN_INDENT_PRINT;
	fputs("enum class _OpCode : uint32_t {", fs); BPCGEN_INDENT_INC;
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
	fputs("};", fs);
}

void codehpp_write_document(FILE* fs, BPCSMTV_DOCUMENT* document) {
	BPCGEN_INDENT_INIT_NEW(fs);

	// write header
	bpcfs_write_snippets(fs, &bpcsnp_hpp_header);

	// write namespace
	GSList* cursor = NULL;
	for (cursor = document->namespace_data; cursor != NULL; cursor = cursor->next) {
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "namespace %s {", (char*)cursor->data); BPCGEN_INDENT_INC;
	}

	// write opcode
	GSList* msg_ls = bpcgen_constructor_msg_list(document->protocol_body);
	write_opcode_enum(fs, msg_ls, BPCGEN_INDENT_REF);

	// write functions snippets
	bpcfs_write_snippets(fs, &bpcsnp_hpp_functions);

	// iterate list to get data
	BPCGEN_STRUCT_LIKE struct_like = { 0 };
	for (cursor = document->protocol_body; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_PROTOCOL_BODY* data = (BPCSMTV_PROTOCOL_BODY*)cursor->data;

		switch (data->node_type) {
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS:
				write_alias(fs, data->node_data.alias_data, BPCGEN_INDENT_REF);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM:
				write_enum(fs, data->node_data.enum_data, BPCGEN_INDENT_REF);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT:
				struct_like.is_msg = false;
				struct_like.real_ptr.pStruct = data->node_data.struct_data;
				write_struct_or_msg(fs, &struct_like, BPCGEN_INDENT_REF);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG:
				struct_like.is_msg = true;
				struct_like.real_ptr.pMsg = data->node_data.msg_data;
				write_struct_or_msg(fs, &struct_like, BPCGEN_INDENT_REF);
				break;
			default:
				g_assert_not_reached();
		}
	}

	// free msg list
	bpcgen_destructor_msg_list(msg_ls);

	// namespace over
	for (cursor = document->namespace_data; cursor != NULL; cursor = cursor->next) {
		BPCGEN_INDENT_DEC;
		BPCGEN_INDENT_PRINT;
		fputc('}', fs);
	}

}
