#include "../bpc_code_gen.h"

static const char* cpp_basic_type[] = {
	"float", "double", "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "std::string"
};

static const char* get_primitive_type_name(BPCSMTV_VARIABLE* variable) {
	return (variable->variable_type->is_basic_type ?
		cpp_basic_type[variable->variable_type->type_data.basic_type] :
		(const char*)variable->variable_type->type_data.custom_type);
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
				fullsize += bpcsmtv_get_bt_size(vardata->variable_type->full_uncover_basic_type);
				break;
			}
			case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
			{
				fullsize += bpcsmtv_get_bt_size(vardata->variable_type->full_uncover_basic_type) *
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

		if (vardata->variable_align->use_align) {
			fullsize += vardata->variable_align->padding_size;
		}
	}

	return (fullsize == UINT32_C(0) ? UINT32_C(1) : fullsize);
}

static void write_struct_or_msg(FILE* fs, BPCGEN_STRUCT_LIKE* union_data, BPCGEN_INDENT_TYPE indent) {
	GSList* cursor = NULL;
	bool is_msg;  GSList* variables; BPCSMTV_STRUCT_MODIFIER* modifier; char* struct_like_name;
	bpcgen_pick_struct_like_data(union_data, &is_msg, &variables, &modifier, &struct_like_name);
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

	// class fake Serialize and Deserialize need do extra swap to make sure all data is little endian
	// before real serialize and destrialize. the real worker will only process data as little endian.
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "bool %s::Serialize(std::stringstream* _ss) {", struct_like_name); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::_InnerSwap(&(this->_InternalData));", struct_like_name);	// swap to LE
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "bool hr = %s::_InnerSerialize(&(this->_InternalData), _ss);", struct_like_name);
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::_InnerSwap(&(this->_InternalData));", struct_like_name);	// swap back to host endian
	BPCGEN_INDENT_PRINT;
	fputs("return hr;", fs);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	BPCGEN_INDENT_PRINT;
	fprintf(fs, "bool %s::Deserialize(std::stringstream* _ss) {", struct_like_name); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "bool hr = %s::_InnerDeserialize(&(this->_InternalData), _ss);", struct_like_name);
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::_InnerSwap(&(this->_InternalData));", struct_like_name);	// swap to host endian
	BPCGEN_INDENT_PRINT;
	fputs("return hr;", fs);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// msg specific functions
	if (is_msg) {
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "_OpCode %s::GetOpCode() { return _OpCode::%s; }", struct_like_name, struct_like_name);

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
					fprintf(fs, "for (uint32_t c = 0; c < UINT32_C(%" PRIu32 "); ++c) { _p->%s[c] = \"\"; }",
						vardata->variable_array->static_array_len,
						vardata->variable_name
					);
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
					fprintf(fs, "for (uint32_t c = 0; c < UINT32_C(%" PRIu32 "); ++c) { %s::_InnerConstructor(&(_p->%s[c])); }",
						vardata->variable_array->static_array_len,

						vardata->variable_type->type_data.custom_type,
						vardata->variable_name
					);
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
				// dynamic array need manual free
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (auto it = _p->%s.begin(); it != _p->%s.end(); ++it) { delete (*it); }", 
						vardata->variable_name, 
						vardata->variable_name
					);
					break;
				}

				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					// dynamic natural do not need call any destructor. bucause nothing need to destructor.
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (auto it = _p->%s.begin(); it != _p->%s.end(); ++it) { delete (*it); }", 
						vardata->variable_name, 
						vardata->variable_name
					);
					break;
				}

				// narrow struct variable need manual free
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s::_InnerDestructor(&(_p->%s));", vardata->variable_type->type_data.custom_type, vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t c = 0; c < UINT32_C(%" PRIu32 "); ++c) { %s::_InnerDestructor(&(_p->%s[c])); }", 
						vardata->variable_array->static_array_len,

						vardata->variable_type->type_data.custom_type, 
						vardata->variable_name
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (auto it = _p->%s.begin(); it != _p->%s.end(); ++it) { %s::_InnerDestructor(*it); delete (*it); }", 
						vardata->variable_name, 
						vardata->variable_name,
					
						vardata->variable_type->type_data.custom_type
					);
					break;
				}


				// these variables do not need free
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_STRING:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
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


	// real swap
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "void %s::_InnerSwap(_InternalDataType* _p) {", struct_like_name); BPCGEN_INDENT_INC;
	// avoid unecessary swap
	BPCGEN_INDENT_PRINT;
	fputs("if (_EndianHelper::IsLittleEndian()) return;", fs);
	// swap each variables
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			BPCSMTV_VARIABLE* vardata = data->plist_vars[c];
			switch (data->vars_type[c]) {
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_EndianHelper::SwapEndian%" PRIu32 "(&(_p->%s));", 
						bpcsmtv_get_bt_size(vardata->variable_type->full_uncover_basic_type) * 8,
						vardata->variable_name
					);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_EndianHelper::SwapEndianArray%" PRIu32 "(&(_p->%s), UINT32_C(%" PRIu32 "));",
						bpcsmtv_get_bt_size(vardata->variable_type->full_uncover_basic_type) * 8,
						vardata->variable_name,
						vardata->variable_array->static_array_len
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_EndianHelper::SwapEndianArray%" PRIu32 "(_p->%s.data(), (uint32_t)_p->%s.size());",
						bpcsmtv_get_bt_size(vardata->variable_type->full_uncover_basic_type) * 8,
						vardata->variable_name,
						vardata->variable_name
					);
					break;
				}

				// narrow and natural share same swap code
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s::_InnerSwap(&(_p->%s));", vardata->variable_type->type_data.custom_type, vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t c = 0; c < UINT32_C(%" PRIu32 "); ++c) { %s::_InnerSwap(&(_p->%s[c])); }",
						vardata->variable_array->static_array_len,

						vardata->variable_type->type_data.custom_type,
						vardata->variable_name
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (auto it = _p->%s.begin(); it != _p->%s.end(); ++it) { %s::_InnerSwap(*it); }",
						vardata->variable_name,
						vardata->variable_name,

						vardata->variable_type->type_data.custom_type
					);
					break;
				}


				// string variables do not need swap
				case BPCGEN_VARTYPE_SINGLE_STRING:
				case BPCGEN_VARTYPE_STATIC_STRING:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
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
		fprintf(fs, "if (_opcode_checker != _OpCode::%s) return false;", struct_like_name);
	}
	// may be used variables
	BPCGEN_INDENT_PRINT;
	fputs("uint32_t _count;", fs);
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;

		// annotations
		BPCGEN_INDENT_PRINT;
		bpcgen_print_variables_annotation(fs, "// ", data);

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
					BPCGEN_INDENT_PRINT;
					fputs("SSTREAM_RD_STRUCT(_ss, sizeof(uint32_t), &_count);", fs);
					BPCGEN_INDENT_PRINT;
					fputs("_EndianHelper::SwapEndian32(&_count);", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_p->%s.resize(_count);", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "SSTREAM_RD_STRUCT(_ss, _count * sizeof(%s), _p->%s.data());", 
						get_primitive_type_name(vardata), 
						vardata->variable_name
					);
					break;
				}

				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_Helper::ReadString(_ss, &(_p->%s));", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t c = 0; c < UINT32_C(%" PRIu32 "); ++c) { _Helper::ReadString(_ss, &(_p->%s[c])); }", 
						vardata->variable_array->static_array_len,
					
						vardata->variable_name
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fputs("SSTREAM_RD_STRUCT(_ss, sizeof(uint32_t), &_count);", fs);
					BPCGEN_INDENT_PRINT;
					fputs("_EndianHelper::SwapEndian32(&_count);", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_Helper::ResizePtrVector<std::string>(&(_p->%s), _count, NULL, NULL);", vardata->variable_name);

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t c = 0; c < _count; ++c) { _Helper::ReadString(_ss, _p->%s[c]); }",
						vardata->variable_name
					);
					break;
				}

				// natural and narrow is shared
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s::_InnerDeserialize(&(_p->%s), _ss);", vardata->variable_type->type_data.custom_type, vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t c = 0; c < UINT32_C(%" PRIu32 "); ++c) { %s::_InnerDeserialize(&(_p->%s[c]), _ss); }",
						vardata->variable_array->static_array_len,

						vardata->variable_type->type_data.custom_type,
						vardata->variable_name
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fputs("SSTREAM_RD_STRUCT(_ss, sizeof(uint32_t), &_count);", fs);
					BPCGEN_INDENT_PRINT;
					fputs("_EndianHelper::SwapEndian32(&_count);", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_Helper::ResizePtrVector<%s::_InternalDataType>(&(_p->%s), _count, &%s::_InnerConstructor, &%s::_InnerDestructor);", 
						vardata->variable_type->type_data.custom_type,
						vardata->variable_name,
						vardata->variable_type->type_data.custom_type,
						vardata->variable_type->type_data.custom_type
					);

					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t c = 0; c < _count; ++c) { %s::_InnerDeserialize(_p->%s[c], _ss); }",
						vardata->variable_type->type_data.custom_type,
						vardata->variable_name
					);
					break;
				}

				// these variable has been processed
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				default:
					g_assert_not_reached();
			}

			// align data
			if (vardata->variable_align->use_align) {
				BPCGEN_INDENT_PRINT;
				fprintf(fs, "_ss->seekg(%" PRIu32 ", std::ios_base::cur);", vardata->variable_align->padding_size);
			}
		}
	}
	BPCGEN_INDENT_PRINT;
	fputs("return true;", fs);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// real serializer
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "bool %s::_InnerSerialize(_InternalDataType* _p, std::stringstream* _ss) {", struct_like_name); BPCGEN_INDENT_INC;
	// msg specific stmt
	if (is_msg) {
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "_Helper::WriteOpCode(_ss, _OpCode::%s);", struct_like_name);
	}
	// may be used variables
	BPCGEN_INDENT_PRINT;
	fputs("uint32_t _count;", fs);
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;

		// annotations
		BPCGEN_INDENT_PRINT;
		bpcgen_print_variables_annotation(fs, "// ", data);

		if (data->is_bonded) {
			// bond vars
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "SSTREAM_WR_STRUCT(_ss, %" PRIu32 ", &(_p->%s));",
				calc_bond_vars_size(data),
				data->plist_vars[0]->variable_name);

		} else {
			// normal process
			BPCSMTV_VARIABLE* vardata = data->plist_vars[0];

			// body
			switch (data->vars_type[0]) {
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_count = _p->%s.size();", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fputs("_EndianHelper::SwapEndian32(&_count);", fs);
					BPCGEN_INDENT_PRINT;
					fputs("SSTREAM_WR_STRUCT(_ss, sizeof(uint32_t), &_count);", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "SSTREAM_WR_STRUCT(_ss, _count * sizeof(%s), _p->%s.data());",
						get_primitive_type_name(vardata),
						vardata->variable_name
					);
					break;
				}

				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_Helper::WriteString(_ss, &(_p->%s));", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t c = 0; c < UINT32_C(%" PRIu32 "); ++c) { _Helper::WriteString(_ss, &(_p->%s[c])); }",
						vardata->variable_array->static_array_len,

						vardata->variable_name
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_count = _p->%s.size();", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fputs("_EndianHelper::SwapEndian32(&_count);", fs);
					BPCGEN_INDENT_PRINT;
					fputs("SSTREAM_WR_STRUCT(_ss, sizeof(uint32_t), &_count);", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t c = 0; c < _p->%s.size(); ++c) { _Helper::WriteString(_ss, _p->%s[c]); }",
						vardata->variable_name,
						vardata->variable_name
					);
					break;
				}

				// natural and narrow is shared
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s::_InnerSerialize(&(_p->%s), _ss);", vardata->variable_type->type_data.custom_type, vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t c = 0; c < UINT32_C(%" PRIu32 "); ++c) { %s::_InnerSerialize(&(_p->%s[c]), _ss); }",
						vardata->variable_array->static_array_len,

						vardata->variable_type->type_data.custom_type,
						vardata->variable_name
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_count = _p->%s.size();", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fputs("_EndianHelper::SwapEndian32(&_count);", fs);
					BPCGEN_INDENT_PRINT;
					fputs("SSTREAM_WR_STRUCT(_ss, sizeof(uint32_t), &_count);", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t c = 0; c < _p->%s.size(); ++c) { %s::_InnerSerialize(_p->%s[c], _ss); }",
						vardata->variable_name,
						vardata->variable_type->type_data.custom_type,
						vardata->variable_name
					);
					break;
				}

				// these variable has been processed
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				default:
					g_assert_not_reached();
			}

			// align data
			if (vardata->variable_align->use_align) {
				BPCGEN_INDENT_PRINT;
				fprintf(fs, "_ss->seekp(%" PRIu32 ", std::ios_base::cur);", vardata->variable_align->padding_size);
			}
		}
	}
	BPCGEN_INDENT_PRINT;
	fputs("return true;", fs);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// free all cache data
	bpcgen_destructor_bond_vars(bond_vars);
}

static void write_uniform_deserialize(FILE* fs, GSList* msg_ls, BPCGEN_INDENT_TYPE indent) {
	GSList* cursor;
	BPCGEN_INDENT_INIT_REF(fs, indent);

	// write uniformed deserialize func
	BPCGEN_INDENT_PRINT;
	fputs("_BpMessage* _Helper::UniformDeserializer(std::stringstream* ss) {", fs); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fputs("_OpCode code; PeekOpCode(ss, &code);", fs);
	BPCGEN_INDENT_PRINT;
	fputs("switch (code) {", fs); BPCGEN_INDENT_INC;
	for (cursor = msg_ls; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_MSG* data = (BPCSMTV_MSG*)cursor->data;

		BPCGEN_INDENT_PRINT;
		fprintf(fs, "case _OpCode::%s: {", data->msg_name);

		// write if body
		BPCGEN_INDENT_INC;
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "auto _data = new %s();", data->msg_name);
		BPCGEN_INDENT_PRINT;
		fputs("_data->Deserialize(ss);", fs);
		BPCGEN_INDENT_PRINT;
		fputs("return _data;", fs);

		BPCGEN_INDENT_DEC;
		BPCGEN_INDENT_PRINT;
		fputc('}', fs);
	}
	// default return
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "default: return NULL;");
	// switch over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);


	// uniform func is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

}

void codecpp_write_document(FILE* fs, BPCSMTV_DOCUMENT* document, const gchar* hpp_reference) {
	BPCGEN_INDENT_INIT_NEW(fs);

	// write hpp reference
	fprintf(fs, "#include \"%s\"", hpp_reference);

	// write namespace
	GSList* cursor = NULL;
	for (cursor = document->namespace_data; cursor != NULL; cursor = cursor->next) {
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "namespace %s {", (char*)cursor->data); BPCGEN_INDENT_INC;
	}

	// write functions snippets
	bpcfs_write_snippets(fs, &bpcsnp_cpp_functions);

	// iterate list to get data
	BPCGEN_STRUCT_LIKE struct_like = { 0 };
	for (cursor = document->protocol_body; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_PROTOCOL_BODY* data = (BPCSMTV_PROTOCOL_BODY*)cursor->data;

		switch (data->node_type) {
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
			
			// alias and enum has been written in header, skip
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS:
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM:
				break;
			default:
				g_assert_not_reached();
		}
	}

	// write uniform_deserializer
	GSList* msg_ls = bpcgen_constructor_msg_list(document->protocol_body);
	write_uniform_deserialize(fs, msg_ls, BPCGEN_INDENT_REF);
	bpcgen_destructor_msg_list(msg_ls);

	// write tail snippets
	bpcfs_write_snippets(fs, &bpcsnp_cpp_tail);

	// namespace over
	for (cursor = document->namespace_data; cursor != NULL; cursor = cursor->next) {
		BPCGEN_INDENT_DEC;
		BPCGEN_INDENT_PRINT;
		fputc('}', fs);
	}

}
