#include "../bpc_code_gen.h"

static const uint32_t cpp_basic_type_size[] = {
	UINT32_C(4), UINT32_C(8), UINT32_C(1), UINT32_C(2), UINT32_C(4), UINT32_C(8), UINT32_C(1), UINT32_C(2), UINT32_C(4), UINT32_C(8)
};
static const char* cpp_basic_type[] = {
	"float", "double", "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "std::string"
};

static char* get_primitive_type_name(BPCSMTV_VARIABLE* variable) {
	return (variable->variable_type->is_basic_type ?
		cpp_basic_type[variable->variable_type->type_data.basic_type] :
		variable->variable_type->type_data.custom_type);
}

static void print_bond_vars_annotation(FILE* fs, BOND_VARS* data) {
	fputs("// ", fs);

	if (data->is_bonded) {
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			if (c != 0) {
				fputs(", ", fs);
			}

			BPCSMTV_VARIABLE* vardata = data->plist_vars[c];
			fputs(vardata->variable_name, fs);
		}
	} else {
		BPCSMTV_VARIABLE* vardata = data->plist_vars[0];
		fputs(vardata->variable_name, fs);
	}
}

static uint32_t get_struct_size(const char* name) {
	BPCSMTV_PROTOCOL_BODY* body = bpcsmtv_registery_identifier_get(name);
	g_assert(body != NULL);
	g_assert(body->node_type == BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT);

	return body->node_data.struct_data->struct_modifier->struct_size;
}

/// <summary>
/// calculate bond vars size.
/// </summary>
/// <param name="data"></param>
/// <returns></returns>
static uint32_t calc_bond_vars_size(BOND_VARS* data) {
	g_assert(data->is_bonded);

	uint32_t fullsize = UINT32_C(0);
	uint32_t c;
	for (c = 0; c < data->bond_vars_len; ++c) {
		BPCSMTV_VARIABLE* vardata = data->plist_vars[c];
		switch (data->vars_type[c]) {
			case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
			{
				fullsize += cpp_basic_type_size[vardata->variable_type->full_uncover_basic_type];
				break;
			}
			case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
			{
				fullsize += cpp_basic_type_size[vardata->variable_type->full_uncover_basic_type] *
					vardata->variable_array->static_array_len;
				break;
			}
			case BPCGEN_VARTYPE_SINGLE_NATURAL:
			{
				fullsize += get_struct_size(vardata->variable_type->type_data.custom_type);
				break;
			}
			case BPCGEN_VARTYPE_STATIC_NATURAL:
			{
				fullsize += get_struct_size(vardata->variable_type->type_data.custom_type) *
					vardata->variable_array->static_array_len;
				break;
			}

			case BPCGEN_VARTYPE_SINGLE_STRING:
			case BPCGEN_VARTYPE_SINGLE_NARROW:
			case BPCGEN_VARTYPE_STATIC_STRING:
			case BPCGEN_VARTYPE_STATIC_NARROW:
			case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
			case BPCGEN_VARTYPE_DYNAMIC_STRING:
			case BPCGEN_VARTYPE_DYNAMIC_NARROW:
			case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
			default:
				g_assert_not_reached();
		}
	}

	return (fullsize == UINT32_C(0) ? UINT32_C(1) : fullsize);
}

static void write_struct_or_msg(FILE* fs, BPCGEN_STRUCT_LIKE* union_data, bool is_msg, BPCGEN_INDENT_TYPE indent) {
	GSList* cursor = NULL, * variables = (is_msg ? union_data->pMsg->msg_body : union_data->pStruct->struct_body);
	BPCSMTV_STRUCT_MODIFIER* modifier = (is_msg ? union_data->pMsg->msg_modifier : union_data->pStruct->struct_modifier);
	char* struct_like_name = (is_msg ? union_data->pMsg->msg_name : union_data->pStruct->struct_name);
	BPCGEN_INDENT_INIT_REF(fs, indent);

	// in c++, we put single primitive and static primitive together
	GSList* bond_vars = bpcgen_constructor_bond_vars(variables, 
		BPCGEN_VARTYPE_SINGLE_PRIMITIVE | BPCGEN_VARTYPE_SINGLE_NATURAL | BPCGEN_VARTYPE_STATIC_PRIMITIVE | BPCGEN_VARTYPE_STATIC_NATURAL);

	// some fake functions, which are just calling real functions
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::%s() : _InternalData() {", struct_like_name, struct_like_name); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::_InnerConstructor(&(this->_InternalData));", struct_like_name);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::~%s() {", struct_like_name, struct_like_name); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::_InnerDestructor(&(this->_InternalData));", struct_like_name);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	BPCGEN_INDENT_PRINT;
	fprintf(fs, "bool %s::Serialize(std::stringstream* _ss) {", struct_like_name); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::_InnerSerialize(&(this->_InternalData), _ss);", struct_like_name);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	BPCGEN_INDENT_PRINT;
	fprintf(fs, "bool %s::Deserialize(std::stringstream* _ss) {", struct_like_name); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::_InnerDeserialize(&(this->_InternalData), _ss);", struct_like_name);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// msg specific functions
	if (is_msg) {
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "_OpCode %s::GetOpCode() { return _OpCode.%s; }", struct_like_name, struct_like_name);

		BPCGEN_INDENT_PRINT;
		fprintf(fs, "bool %s::GetIsReliable() { return %s; }", struct_like_name, 
			(modifier->is_reliable ? "true" : "false"));
	}

	// real constructor
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "void %s::_InnerConstructor(_InternalDataType* _p) {", struct_like_name); BPCGEN_INDENT_INC;
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			BPCSMTV_VARIABLE* vardata = data->plist_vars[c];
			switch (data->vars_type[c]) {
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "memset(&(_p->%s), 0, sizeof(%s));", vardata->variable_name, get_primitive_type_name(vardata));
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "memset(&(_p->%s), 0, sizeof(%s) * %" PRIu32 ");", 
						vardata->variable_name, 
						get_primitive_type_name(vardata),
						vardata->variable_array->static_array_len);
					break;
				}

				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_p->%s = \"\";", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t c = 0; c < UINT32_C(%" PRIu32 "); ++c) {", vardata->variable_array->static_array_len); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_p->%s[c] = \"\";", vardata->variable_name);
					BPCGEN_INDENT_DEC;
					BPCGEN_INDENT_PRINT;
					fputc('}', fs);
					break;
				}

				case BPCGEN_VARTYPE_SINGLE_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s::_InnerConstructor(&(_p->%s));", vardata->variable_type->type_data.custom_type, vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t c = 0; c < UINT32_C(%" PRIu32 "); ++c) {", vardata->variable_array->static_array_len); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s::_InnerConstructor(&(_p->%s[c]));", vardata->variable_type->type_data.custom_type, vardata->variable_name);
					BPCGEN_INDENT_DEC;
					BPCGEN_INDENT_PRINT;
					fputc('}', fs);
					break;
				}

				// these variables do not need init
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
					break;
				default:
					g_assert_not_reached();
			}

		}
	}
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// real destructor
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "void %s::_InnerDestructor(_InternalDataType* _p) {", struct_like_name); BPCGEN_INDENT_INC;
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			BPCSMTV_VARIABLE* vardata = data->plist_vars[c];
			switch (data->vars_type[c]) {
				// non-primitive variable need manual free
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s::_InnerDetructor(&(_p->%s));", vardata->variable_type->type_data.custom_type, vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s::_InnerDestructor(&(_p->%s[c]));", vardata->variable_type->type_data.custom_type, vardata->variable_name);
					break;
				}

				// dynamic array need manual free
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (auto it = _p->%s.begin(); it != _ps->%s.end(); ++it) {", vardata->variable_name, vardata->variable_name); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fputs("delete (*it);", fs);
					BPCGEN_INDENT_DEC;
					BPCGEN_INDENT_PRINT;
					fputc('}', fs);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (auto it = _p->%s.begin(); it != _p->%s.end(); ++it) {", vardata->variable_name, vardata->variable_name); BPCGEN_INDENT_INC;
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s::_InnerDestructor(it);", vardata->variable_type->type_data.custom_type);
					BPCGEN_INDENT_PRINT;
					fputs("delete (*it);", fs);
					BPCGEN_INDENT_DEC;
					BPCGEN_INDENT_PRINT;
					fputc('}', fs);
				}

				// these variables do not need free
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_STRING:
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
					break;
				default:
					g_assert_not_reached();
			}

		}
	}
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// real deserializer
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "bool %s::_InnerDeserialize(_InternalDataType* _p, std::stringstream* _ss) {", struct_like_name); BPCGEN_INDENT_INC;
	// msg specific stmt
	if (is_msg) {
		BPCGEN_INDENT_PRINT;
		fputs("_OpCode _opcode_checker;", fs);
		BPCGEN_INDENT_PRINT;
		fputs("_Helper::ReadOpCode(_ss, &_opcode_checker);", fs);
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "if (_opcode_checker != _OpCode.%s) return false;", struct_like_name);
	}
	// may be used variables
	BPCGEN_INDENT_PRINT;
	fputs("uint32_t _len, _count;", fs);
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;

		BPCGEN_INDENT_PRINT;
		print_bond_vars_annotation(fs, data);

		if (data->is_bonded) {
			// bond vars
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "SSTREAM_RD_STRUCT(_ss, %" PRIu32 ", &(_p->%s));", 
				calc_bond_vars_size(data),
				data->plist_vars[0]->variable_name);

		} else {
			// normal process
			BPCSMTV_VARIABLE* vardata = data->plist_vars[0];

			// body
			switch (data->vars_type[0]) {
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				{

					break;
				}

				case BPCGEN_VARTYPE_SINGLE_STRING:
				{

					break;
				}
				case BPCGEN_VARTYPE_STATIC_STRING:
				{

					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{

					break;
				}

				// natural and narrow is shared
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				{

					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				{

					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{

					break;
				}

				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				default:
					g_assert_not_reached();
			}
		}
	}
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// real serializer
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "bool %s::_InnerSerialize(_InternalDataType* _p, std::stringstream* _ss) {", struct_like_name); BPCGEN_INDENT_INC;
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;

		BPCGEN_INDENT_PRINT;
		print_bond_vars_annotation(fs, data);

		if (data->is_bonded) {

		} else {

		}
	}
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

}

void codecpp_write_document(FILE* fs, BPCSMTV_DOCUMENT* document, const gchar* hpp_reference) {

}
