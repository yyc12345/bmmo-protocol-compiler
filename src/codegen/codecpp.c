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

/// <summary>
/// check the bond_vars to decide whether adding _count for dynamic list RW.
/// </summary>
/// <param name="bond_vars">GSList. item is BOND_VAR*</param>
/// <returns>return true if _count is necessary.</returns>
static bool is_need_var_count(GSList* bond_vars) {
	GSList* cursor = NULL;
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			switch (data->vars_type[c]) {
				// only dynamic list need bond var
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					return true;
				}
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_STRING:
				case BPCGEN_VARTYPE_STATIC_STRING:
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					break;	// do nothing in other types. check next one
				}

				default:
					g_assert_not_reached();
			}

		}
	}

	// no dynamic object
	return false;
}

static void write_struct_or_msg(FILE* fs, BPCGEN_STRUCT_LIKE* union_data, BPCGEN_INDENT_TYPE indent) {
	GSList* cursor = NULL;
	bool is_msg;  GSList* variables; BPCSMTV_STRUCT_MODIFIER* modifier; char* struct_like_name;
	bpcgen_pick_struct_like_data(union_data, &is_msg, &variables, &modifier, &struct_like_name);
	BPCGEN_INDENT_INIT_REF(fs, indent);

	// in c++, we put single primitive and static primitive together
	GSList* bond_vars = bpcgen_constructor_bond_vars(variables,
		BPCGEN_VARTYPE_SINGLE_PRIMITIVE | BPCGEN_VARTYPE_SINGLE_NATURAL | BPCGEN_VARTYPE_STATIC_PRIMITIVE | BPCGEN_VARTYPE_STATIC_NATURAL);

	// compute whether need _count for following using.
	bool need_extra_var = is_need_var_count(bond_vars);

	// real constructor
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::Payload_t::Payload_t()", struct_like_name);
	if (variables != NULL) { fputs(" : ", fs); }	// if no member. we do not need put init list start symbol.
	for (cursor = variables; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_VARIABLE* vardata = (BPCSMTV_VARIABLE*)cursor->data;

		// all variable use the same init style
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "%s()", vardata->variable_name);

		// if not the last one. add `,`
		if (cursor->next != NULL) {
			fputc(',', fs);
		}
	}
	BPCGEN_INDENT_PRINT;
	fputs("{}", fs);

	// real destructor
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::Payload_t::~Payload_t() {}", struct_like_name);

	// real copy constructor
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::Payload_t::Payload_t(const %s::Payload_t& _rhs)", struct_like_name, struct_like_name);
	if (variables != NULL) { fputs(" : ", fs); }
	for (cursor = variables; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_VARIABLE* vardata = (BPCSMTV_VARIABLE*)cursor->data;

		// all variable use the same init style
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "%s(_rhs.%s)", vardata->variable_name, vardata->variable_name);

		// if not the last one. add `,`
		if (cursor->next != NULL) {
			fputc(',', fs);
		}
	}
	BPCGEN_INDENT_PRINT;
	fputs("{}", fs);

	// real move constructor
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::Payload_t::Payload_t(%s::Payload_t&& _rhs) noexcept", struct_like_name, struct_like_name);
	if (variables != NULL) { fputs(" : ", fs); }
	for (cursor = variables; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_VARIABLE* vardata = (BPCSMTV_VARIABLE*)cursor->data;

		// all variable use the same init style
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "%s(std::move(_rhs.%s))", vardata->variable_name, vardata->variable_name);

		// if not the last one. add `,`
		if (cursor->next != NULL) {
			fputc(',', fs);
		}
	}
	BPCGEN_INDENT_PRINT;
	fputs("{}", fs);

	// real copy operator=
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::Payload_t& %s::Payload_t::operator=(const %s::Payload_t& _rhs) {", struct_like_name, struct_like_name, struct_like_name); BPCGEN_INDENT_INC;
	for (cursor = variables; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_VARIABLE* vardata = (BPCSMTV_VARIABLE*)cursor->data;

		// all variable use the same style
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "%s = _rhs.%s;", vardata->variable_name, vardata->variable_name);

	}
	BPCGEN_INDENT_PRINT;
	fputs("return *this;", fs);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// real move operator=
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s::Payload_t& %s::Payload_t::operator=(%s::Payload_t&& _rhs) noexcept {", struct_like_name, struct_like_name, struct_like_name); BPCGEN_INDENT_INC;
	for (cursor = variables; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_VARIABLE* vardata = (BPCSMTV_VARIABLE*)cursor->data;

		// all variable use the same style
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "%s = std::move(_rhs.%s);", vardata->variable_name, vardata->variable_name);

	}
	BPCGEN_INDENT_PRINT;
	fputs("return *this;", fs);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// real swap
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "void %s::Payload_t::ByteSwap() {", struct_like_name); BPCGEN_INDENT_INC;
	// avoid unecessary swap
	BPCGEN_INDENT_PRINT;
	fputs("if _BP_IS_LITTLE_ENDIAN return;", fs);
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
					fprintf(fs, "BPHelper::ByteSwap::_SwapSingle<%s>(&%s);",
						get_primitive_type_name(vardata),
						vardata->variable_name
					);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "BPHelper::ByteSwap::_SwapArray<%s>(%s.data(), UINT32_C(%" PRIu32 "));",
						get_primitive_type_name(vardata),
						vardata->variable_name,
						vardata->variable_array->static_array_len
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "BPHelper::ByteSwap::_SwapArray<%s>(%s.data(), static_cast<uint32_t>(%s.size()));",
						get_primitive_type_name(vardata),
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
					fprintf(fs, "%s.ByteSwap();", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t _c = 0; _c < UINT32_C(%" PRIu32 "); ++_c) { %s[_c].ByteSwap(); }",
						vardata->variable_array->static_array_len,
						vardata->variable_name
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (auto& _it : %s) { _it.ByteSwap(); }",
						vardata->variable_name
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
	fprintf(fs, "bool %s::Payload_t::Deserialize(std::istream& _ss) {", struct_like_name); BPCGEN_INDENT_INC;
	// may be used variables
	if (need_extra_var) {
		BPCGEN_INDENT_PRINT;
		fputs("uint32_t _count;", fs);
	}
	BPCGEN_INDENT_PRINT;
	fputs("_SS_PRE_RD(_ss);", fs);
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;

		// annotations
		BPCGEN_INDENT_PRINT;
		bpcgen_print_variables_annotation(fs, "// ", data);

		if (data->is_bonded) {
			// bond vars
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "_SS_RD_STRUCT(_ss, &%s, %" PRIu32 ");",
				data->plist_vars[0]->variable_name,
				calc_bond_vars_size(data)
			);

		} else {
			// normal process
			BPCSMTV_VARIABLE* vardata = data->plist_vars[0];

			// body
			switch (data->vars_type[0]) {
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fputs("_SS_RD_STRUCT(_ss, &_count, sizeof(uint32_t));", fs);
					BPCGEN_INDENT_PRINT;
					fputs("if _BP_IS_BIG_ENDIAN { BPHelper::ByteSwap::_SwapSingle<uint32_t>(&_count); }", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s.resize(_count);", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_SS_RD_STRUCT(_ss, %s.data(), %s.size() * sizeof(%s));",
						vardata->variable_name,
						vardata->variable_name,
						get_primitive_type_name(vardata)
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fputs("_SS_RD_STRUCT(_ss, &_count, sizeof(uint32_t));", fs);
					BPCGEN_INDENT_PRINT;
					fputs("if _BP_IS_BIG_ENDIAN { BPHelper::ByteSwap::_SwapSingle<uint32_t>(&_count); }", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s.resize(_count);", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_SS_RD_STRUCT(_ss, %s.data(), %s.size() * sizeof(%s::Payload_t));",
						vardata->variable_name,
						vardata->variable_name,
						get_primitive_type_name(vardata)
					);
					break;
				}

				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_SS_RD_STRING(_ss, %s);", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t _c = 0; _c < UINT32_C(%" PRIu32 "); ++_c) { _SS_RD_STRING(_ss, %s[_c]); }",
						vardata->variable_array->static_array_len,
						vardata->variable_name
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fputs("_SS_RD_STRUCT(_ss, &_count, sizeof(uint32_t));", fs);
					BPCGEN_INDENT_PRINT;
					fputs("if _BP_IS_BIG_ENDIAN { BPHelper::ByteSwap::_SwapSingle<uint32_t>(&_count); }", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s.resize(_count);", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (auto& _it : %s) { _SS_RD_STRING(_ss, _it); }",
						vardata->variable_name
					);
					break;
				}

				// natural and narrow is shared
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_SS_RD_FUNCTION(_ss, %s);", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t _c = 0; _c < UINT32_C(%" PRIu32 "); ++_c) { _SS_RD_FUNCTION(_ss, %s[_c]); }",
						vardata->variable_array->static_array_len,
						vardata->variable_name
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fputs("_SS_RD_STRUCT(_ss, &_count, sizeof(uint32_t));", fs);
					BPCGEN_INDENT_PRINT;
					fputs("if _BP_IS_BIG_ENDIAN { BPHelper::ByteSwap::_SwapSingle<uint32_t>(&_count); }", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "%s.resize(_count);", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (auto& _it : %s) { _SS_RD_FUNCTION(_ss, _it); }",
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
				fprintf(fs, "_SS_RD_BLANK(_ss, %" PRIu32 ");", vardata->variable_align->padding_size);
			}
		}
	}
	if (bond_vars == NULL) {
		// no variable, read 1 byte anyway. because sizeof(empty_struct) == 1u
		BPCGEN_INDENT_PRINT;
		fputs("_SS_RD_BLANK(_ss, 1);", fs);
	}
	BPCGEN_INDENT_PRINT;
	fputs("_SS_END_RD(_ss);", fs);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// real serializer
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "bool %s::Payload_t::Serialize(std::ostream& _ss) {", struct_like_name); BPCGEN_INDENT_INC;
	// may be used variables
	if (need_extra_var) {
		BPCGEN_INDENT_PRINT;
		fputs("uint32_t _count;", fs);
	}
	BPCGEN_INDENT_PRINT;
	fputs("_SS_PRE_WR(_ss);", fs);
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;

		// annotations
		BPCGEN_INDENT_PRINT;
		bpcgen_print_variables_annotation(fs, "// ", data);

		if (data->is_bonded) {
			// bond vars
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "_SS_WR_STRUCT(_ss, &%s, %" PRIu32 ");",
				data->plist_vars[0]->variable_name,
				calc_bond_vars_size(data)
			);

		} else {
			// normal process
			BPCSMTV_VARIABLE* vardata = data->plist_vars[0];

			// body
			switch (data->vars_type[0]) {
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_count = %s.size();", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fputs("if _BP_IS_BIG_ENDIAN { BPHelper::ByteSwap::_SwapSingle<uint32_t>(&_count); }", fs);
					BPCGEN_INDENT_PRINT;
					fputs("_SS_WR_STRUCT(_ss, &_count, sizeof(uint32_t));", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_SS_WR_STRUCT(_ss, %s.data(), %s.size() * sizeof(%s));",
						vardata->variable_name,
						vardata->variable_name,
						get_primitive_type_name(vardata)
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_count = %s.size();", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fputs("if _BP_IS_BIG_ENDIAN { BPHelper::ByteSwap::_SwapSingle<uint32_t>(&_count); }", fs);
					BPCGEN_INDENT_PRINT;
					fputs("_SS_WR_STRUCT(_ss, &_count, sizeof(uint32_t));", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_SS_WR_STRUCT(_ss, %s.data(), %s.size() * sizeof(%s::Payload_t));",
						vardata->variable_name,
						vardata->variable_name,
						get_primitive_type_name(vardata)
					);
					break;
				}

				case BPCGEN_VARTYPE_SINGLE_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_SS_WR_STRING(_ss, %s);", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t _c = 0; _c < UINT32_C(%" PRIu32 "); ++_c) { _SS_WR_STRING(_ss, %s[_c]); }",
						vardata->variable_array->static_array_len,
						vardata->variable_name
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_count = %s.size();", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fputs("if _BP_IS_BIG_ENDIAN { BPHelper::ByteSwap::_SwapSingle<uint32_t>(&_count); }", fs);
					BPCGEN_INDENT_PRINT;
					fputs("_SS_WR_STRUCT(_ss, &_count, sizeof(uint32_t));", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (auto& _it : %s) { _SS_WR_STRING(_ss, _it); }",
						vardata->variable_name
					);
					break;
				}

				// natural and narrow is shared
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_SS_WR_FUNCTION(_ss, %s);", vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (uint32_t _c = 0; _c < UINT32_C(%" PRIu32 "); ++_c) { _SS_WR_FUNCTION(_ss, %s[_c]); }",
						vardata->variable_array->static_array_len,
						vardata->variable_name
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "_count = %s.size();", vardata->variable_name);
					BPCGEN_INDENT_PRINT;
					fputs("if _BP_IS_BIG_ENDIAN { BPHelper::ByteSwap::_SwapSingle<uint32_t>(&_count); }", fs);
					BPCGEN_INDENT_PRINT;
					fputs("_SS_WR_STRUCT(_ss, &_count, sizeof(uint32_t));", fs);
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "for (auto& _it : %s) { _SS_WR_FUNCTION(_ss, _it); }",
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
				fprintf(fs, "_SS_WR_BLANK(_ss, %" PRIu32 ");", vardata->variable_align->padding_size);
			}
		}
	}
	if (bond_vars == NULL) {
		// no variable, write 1 byte anyway. because sizeof(empty_struct) == 1u
		BPCGEN_INDENT_PRINT;
		fputs("_SS_WR_BLANK(_ss, 1);", fs);
	}
	BPCGEN_INDENT_PRINT;
	fputs("_SS_END_WR(_ss);", fs);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// free all cache data
	bpcgen_destructor_bond_vars(bond_vars);
}

static void write_msg_factory(FILE* fs, GSList* msg_ls, BPCGEN_INDENT_TYPE indent) {
	GSList* cursor;
	BPCGEN_INDENT_INIT_REF(fs, indent);

	// write uniformed deserialize func
	BPCGEN_INDENT_PRINT;
	fputs("BpMessage* MessageFactory(OpCode code) {", fs); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fputs("switch (code) {", fs); BPCGEN_INDENT_INC;
	for (cursor = msg_ls; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_MSG* data = (BPCSMTV_MSG*)cursor->data;

		// return new instance
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "case OpCode::%s: return new %s;", data->msg_name, data->msg_name);
	}
	// default return
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "default: return nullptr;");
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
	BPCGEN_INDENT_PRINT;
	fputs("namespace ", fs);
	bpcgen_print_join_gslist(fs, "::", true, document->namespace_data);
	fputs(" {", fs); BPCGEN_INDENT_INC;

	// write functions snippets
	bpcfs_write_snippets(fs, &bpcsnp_cpp_functions);

	// iterate list to get data
	GSList* cursor = NULL;
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

	// write msg factory
	GSList* msg_ls = bpcgen_constructor_msg_list(document->protocol_body);

	BPCGEN_INDENT_PRINT;
	fputs("namespace BPHelper {", fs); BPCGEN_INDENT_INC;
	write_msg_factory(fs, msg_ls, BPCGEN_INDENT_REF);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	bpcgen_destructor_msg_list(msg_ls);

	// write tail snippets
	bpcfs_write_snippets(fs, &bpcsnp_cpp_tail);

	// namespace over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

}
