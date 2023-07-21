#include "../bpc_code_gen.h"

static const char* cpp_basic_type[] = {
	"float", "double", "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "std::string"
};
static const char* cpp_testbench_type[] = {
	"f32", "f64", "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "bpstr"
};

static const char* get_primitive_type_name(BPCSMTV_VARIABLE* variable) {
	return (variable->variable_type->is_basic_type ?
		cpp_basic_type[variable->variable_type->type_data.basic_type] :
		(const char*)variable->variable_type->type_data.custom_type);
}

static void write_alias(FILE* fs, BPCSMTV_ALIAS* smtv_alias, BPCGEN_INDENT_TYPE indent) {
	BPCGEN_INDENT_INIT_REF(fs, indent);

	BPCGEN_INDENT_PRINT;
	fprintf(fs, "using %s = %s;", smtv_alias->custom_type, cpp_basic_type[smtv_alias->basic_type]);
}

static void write_enum(FILE* fs, BPCSMTV_ENUM* smtv_enum, BPCGEN_INDENT_TYPE indent) {
	BPCGEN_INDENT_INIT_REF(fs, indent);
	GSList* cursor;

	BPCGEN_INDENT_PRINT;
	fprintf(fs, "enum class %s : %s {", smtv_enum->enum_name, cpp_basic_type[smtv_enum->enum_basic_type]); BPCGEN_INDENT_INC;
	for (cursor = smtv_enum->enum_body; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_ENUM_MEMBER* data = (BPCSMTV_ENUM_MEMBER*)cursor->data;

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
		fprintf(fs, "class %s : public BpMessage {", struct_like_name);
	} else {
		fprintf(fs, "class %s : public BpStruct {", struct_like_name);
	}
	BPCGEN_INDENT_PRINT;
	fputs("public:", fs); BPCGEN_INDENT_INC;

	// internal data define
	// pack specific
	// C++ IMPL WARNING:
	// due to C++ shitty design. I can't use template with #pack. Compiler will omit this #pack instruction and do not padding any space for this.
	// so we use _Placeholder to force;y ensure our behavior.
	BPCGEN_INDENT_PRINT;
	fputs("#pragma pack(1)", fs);
	BPCGEN_INDENT_PRINT;
	fputs("struct Payload_t {", fs); BPCGEN_INDENT_INC;
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
					fprintf(fs, "%s::Payload_t %s;", get_primitive_type_name(vardata), vardata->variable_name);
					break;
				}

				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "CStyleArray<%s, %" PRIu32 "> %s;",
						get_primitive_type_name(vardata),
						vardata->variable_array->static_array_len,
						vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "CStyleArray<%s::Payload_t, %" PRIu32 "> %s;",
						get_primitive_type_name(vardata),
						vardata->variable_array->static_array_len,
						vardata->variable_name);
					break;
				}

				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "std::vector<%s> %s;", get_primitive_type_name(vardata), vardata->variable_name);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					BPCGEN_INDENT_PRINT;
					fprintf(fs, "std::vector<%s::Payload_t> %s;", get_primitive_type_name(vardata), vardata->variable_name);
					break;
				}

				default:
					g_assert_not_reached();
			}

			// always do padding, see previouslt metioned c++ warning for detail.
			// because align always reflect variable align even in natural mode.
			// (for the convenience other languages, python and etc)
			if (vardata->variable_align->use_align) {
				BPCGEN_INDENT_PRINT;
				fprintf(fs, "char _Placeholder%" PRIu32 "[%" PRIu32 "];", ph_counter++, vardata->variable_align->padding_size);
			}
		}
	}
	// if no member. insert a placeholder to ensure its size == 1
	if (bond_vars == NULL) {
		BPCGEN_INDENT_PRINT;
		fputs("char _Placeholder0;", fs);
	}

	// internal data define is over
	// start define some basic functions
	// define c++ functions: constructor, destructor, copy constructor, copy =operator, move constructor, move =operator
	BPCGEN_INDENT_PRINT;
	fputs("Payload_t();", fs);
	BPCGEN_INDENT_PRINT;
	fputs("~Payload_t();", fs);
	BPCGEN_INDENT_PRINT;
	fputs("Payload_t(const Payload_t& _rhs);", fs);
	BPCGEN_INDENT_PRINT;
	fputs("Payload_t(Payload_t&& _rhs) noexcept;", fs);
	BPCGEN_INDENT_PRINT;
	fputs("Payload_t& operator=(const Payload_t& _rhs);", fs);
	BPCGEN_INDENT_PRINT;
	fputs("Payload_t& operator=(Payload_t&& _rhs) noexcept;", fs);
	// define serialization functions
	BPCGEN_INDENT_PRINT;
	fputs("bool Serialize(std::ostream& _ss);", fs);
	BPCGEN_INDENT_PRINT;
	fputs("bool Deserialize(std::istream& _ss);", fs);
	BPCGEN_INDENT_PRINT;
	fputs("void ByteSwap();", fs);
	// defination is over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputs("};", fs);
	// restore pack
	BPCGEN_INDENT_PRINT;
	fputs("#pragma pack()", fs);

	// declare internal data
	BPCGEN_INDENT_PRINT;
	fputs("Payload_t Payload;", fs);

	// declare common functions
	// declare c++ related functions
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s() : Payload() {}", struct_like_name);
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "virtual ~%s() {}", struct_like_name);
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s(const %s& rhs) : Payload(rhs.Payload) {}", struct_like_name, struct_like_name);
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s(%s&& rhs) noexcept : Payload(std::move(rhs.Payload)) {}", struct_like_name, struct_like_name);
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s& operator=(const %s& rhs) { this->Payload = rhs.Payload; return *this; }", struct_like_name, struct_like_name);
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "%s& operator=(%s&& rhs) noexcept { this->Payload = std::move(rhs.Payload); return *this; }", struct_like_name, struct_like_name);
	// declare serialization related functions
	BPCGEN_INDENT_PRINT;
	fputs("virtual bool Serialize(std::ostream& _ss) override {", fs); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fputs("Payload_t* pPayload = nullptr;", fs);
	BPCGEN_INDENT_PRINT;
	fputs("if _BP_IS_BIG_ENDIAN { pPayload = new Payload_t(Payload); pPayload->ByteSwap(); }", fs);	// we need create a new payload in big endian and swap it.
	BPCGEN_INDENT_PRINT;
	fputs("else { pPayload = &Payload; }", fs);
	BPCGEN_INDENT_PRINT;
	fputs("bool hr = pPayload->Serialize(_ss);", fs);
	BPCGEN_INDENT_PRINT;
	fputs("if _BP_IS_BIG_ENDIAN { delete pPayload; }", fs);	// and free it when bbig endian.
	BPCGEN_INDENT_PRINT;
	fputs("return hr;", fs);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	BPCGEN_INDENT_PRINT;
	fputs("virtual bool Deserialize(std::istream& _ss) override {", fs); BPCGEN_INDENT_INC;
	BPCGEN_INDENT_PRINT;
	fputs("bool hr = Payload.Deserialize(_ss);", fs);
	BPCGEN_INDENT_PRINT;
	fputs("if _BP_IS_BIG_ENDIAN { Payload.ByteSwap(); }", fs);	// try using constexpr to prevent useless code compile
	BPCGEN_INDENT_PRINT;
	fputs("return hr;", fs);
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

	// msg unique functions
	if (is_msg) {
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "virtual OpCode GetOpCode() override { return OpCode::%s; }", struct_like_name);
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "virtual bool IsReliable() override { return %s; }", (modifier->is_reliable ? "true" : "false"));
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
	fputs("enum class OpCode : uint32_t {", fs); BPCGEN_INDENT_INC;
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

static void write_testbench_data(FILE* fs, BPCGEN_STRUCT_LIKE* union_data, BPCGEN_INDENT_TYPE indent) {
	GSList* cursor = NULL;
	GSList* variables; char* struct_like_name;
	bpcgen_pick_struct_like_data(union_data, NULL, &variables, NULL, &struct_like_name);
	BPCGEN_INDENT_INIT_REF(fs, indent);

	// we just want to get variables type. we do not need bond any variables.
	GSList* bond_vars = bpcgen_constructor_bond_vars(variables, BPCGEN_VARTYPE_NONE);

	// write header
	// struct name, struct property body
	BPCGEN_INDENT_PRINT;
	fprintf(fs, "{ \"%s\", { _BP_OFFSETOF(%s, Payload), {", struct_like_name, struct_like_name);

	// write variable body
	for (cursor = bond_vars; cursor != NULL; cursor = cursor->next) {
		BOND_VARS* data = (BOND_VARS*)cursor->data;
		uint32_t c;
		for (c = 0; c < data->bond_vars_len; ++c) {
			BPCSMTV_VARIABLE* vardata = data->plist_vars[c];

			// write variable name
			BPCGEN_INDENT_PRINT;
			fprintf(fs, "{ \"%s\", ", vardata->variable_name);

			// write container type
			switch (data->vars_type[c]) {
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_STRING:
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				{
					fputs("_BPTestbench_ContainerType::Single, ", fs);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					fputs("_BPTestbench_ContainerType::Tuple, ", fs);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					fputs("_BPTestbench_ContainerType::List, ", fs);
					break;
				}
				default:
					g_assert_not_reached();
			}

			// write basic and complex data type
			// these type store the type of this variable, or, the element's type of this variable.
			// complex variable only valid when basic type is `bpstruct`
			switch (data->vars_type[c]) {
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_STRING:
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					fprintf(fs, "_BPTestbench_BasicType::%s, \"\", ", cpp_testbench_type[vardata->variable_type->full_uncover_basic_type]);
					break;
				}
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					fprintf(fs, "_BPTestbench_BasicType::bpstruct, \"%s\", ", get_primitive_type_name(vardata));
					break;
				}
				default:
					g_assert_not_reached();
			}

			// write offset
			fprintf(fs, "_BP_OFFSETOF(%s::Payload_t, %s), ", 
				struct_like_name,
				vardata->variable_name
			);

			// write unit size
			switch (data->vars_type[c]) {
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_STRING:
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					// single variable just use their types
					fprintf(fs, "sizeof(%s), ",
						get_primitive_type_name(vardata)
					);
					break;
				}
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					fprintf(fs, "sizeof(%s::Payload_t), ",
						get_primitive_type_name(vardata)
					);
					break;
				}
				default:
					g_assert_not_reached();
			}

			// write tuple size
			switch (data->vars_type[c]) {
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_STRING:
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL: 
				{
					fputs("0, ", fs);
					break;
				}
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					fprintf(fs, "%" PRIu32 ", ",
						vardata->variable_array->static_array_len
					);
					break;
				}
				default:
					g_assert_not_reached();
			}

			// write list function ptr. for non-list member, write nullptr
			switch (data->vars_type[c]) {
				case BPCGEN_VARTYPE_SINGLE_PRIMITIVE:
				case BPCGEN_VARTYPE_SINGLE_STRING:
				case BPCGEN_VARTYPE_SINGLE_NARROW:
				case BPCGEN_VARTYPE_SINGLE_NATURAL:
				case BPCGEN_VARTYPE_STATIC_PRIMITIVE:
				case BPCGEN_VARTYPE_STATIC_STRING:
				case BPCGEN_VARTYPE_STATIC_NARROW:
				case BPCGEN_VARTYPE_STATIC_NATURAL:
				{
					fputs("nullptr, nullptr, nullptr", fs);
					break;
				}
				// BUG INFO!
				// because the bug of MSVC, we need reveal its real type in c++. not use delctype() instead.
				// the difference between primitive+string and struct is that struct need Payload_t suffix
				case BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE:
				case BPCGEN_VARTYPE_DYNAMIC_STRING:
				{
					fprintf(fs, "[](void* cls) -> void* { return reinterpret_cast<std::vector<%s>*>(cls)->data(); }, ",
						get_primitive_type_name(vardata)
					);
					fprintf(fs, "[](void* cls, _BPTestbench_VecSize_t n) -> void { reinterpret_cast<std::vector<%s>*>(cls)->resize(n); }, ",
						get_primitive_type_name(vardata)
					);
					fprintf(fs, "[](void* cls) -> _BPTestbench_VecSize_t { return reinterpret_cast<std::vector<%s>*>(cls)->size(); }",
						get_primitive_type_name(vardata)
					);
					break;
				}
				case BPCGEN_VARTYPE_DYNAMIC_NARROW:
				case BPCGEN_VARTYPE_DYNAMIC_NATURAL:
				{
					fprintf(fs, "[](void* cls) -> void* { return reinterpret_cast<std::vector<%s::Payload_t>*>(cls)->data(); }, ",
						get_primitive_type_name(vardata)
					);
					fprintf(fs, "[](void* cls, _BPTestbench_VecSize_t n) -> void { reinterpret_cast<std::vector<%s::Payload_t>*>(cls)->resize(n); }, ",
						get_primitive_type_name(vardata)
					);
					fprintf(fs, "[](void* cls) -> _BPTestbench_VecSize_t { return reinterpret_cast<std::vector<%s::Payload_t>*>(cls)->size(); }",
						get_primitive_type_name(vardata)
					);
					break;
				}
				default:
					g_assert_not_reached();
			}

			// end of variable property
			fputs("},", fs);
		}
	}

	// end of std::pair (for std::map), struct property, and variable properties
	BPCGEN_INDENT_PRINT;
	fputs("}}},", fs);
	
	// free all cache data
	bpcgen_destructor_bond_vars(bond_vars);
}

static void write_msg_list(FILE* fs, GSList* msg_ls, BPCGEN_INDENT_TYPE indent) {
	GSList* cursor;
	BPCGEN_INDENT_INIT_REF(fs, indent);

	// write all message name
	BPCGEN_INDENT_PRINT;
	fputs("const std::vector<std::string> _BPTestbench_MessageList {", fs); BPCGEN_INDENT_INC;
	for (cursor = msg_ls; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_MSG* data = (BPCSMTV_MSG*)cursor->data;

		// write name
		BPCGEN_INDENT_PRINT;
		fprintf(fs, "\"%s\", ", data->msg_name);
	}
	// over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputs("};", fs);

}

void codehpp_write_document(FILE* fs, BPCSMTV_DOCUMENT* document) {
	BPCGEN_INDENT_INIT_NEW(fs);

	// write header
	bpcfs_write_snippets(fs, &bpcsnp_hpp_header);

	// write namespace
	BPCGEN_INDENT_PRINT;
	fputs("namespace ", fs);
	bpcgen_print_join_gslist(fs, "::", true, document->namespace_data);
	fputs(" {", fs); BPCGEN_INDENT_INC;

	// write opcode
	GSList* msg_ls = bpcgen_constructor_msg_list(document->protocol_body);
	write_opcode_enum(fs, msg_ls, BPCGEN_INDENT_REF);

	// write functions snippets
	bpcfs_write_snippets(fs, &bpcsnp_hpp_functions);

	// iterate list to get data
	GSList* cursor = NULL;
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

	// ===== start testbench code. write macro
	BPCGEN_INDENT_PRINT;
	fputs("#if _ENABLE_BP_TESTBENCH", fs);
	// iterate list to generate testbench data
	BPCGEN_INDENT_PRINT;
	fputs("const std::map<std::string, _BPTestbench_StructProperty> _BPTestbench_StructLayouts {", fs); BPCGEN_INDENT_INC;
	for (cursor = document->protocol_body; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_PROTOCOL_BODY* data = (BPCSMTV_PROTOCOL_BODY*)cursor->data;

		switch (data->node_type) {
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT:
				struct_like.is_msg = false;
				struct_like.real_ptr.pStruct = data->node_data.struct_data;
				write_testbench_data(fs, &struct_like, BPCGEN_INDENT_REF);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG:
				struct_like.is_msg = true;
				struct_like.real_ptr.pMsg = data->node_data.msg_data;
				write_testbench_data(fs, &struct_like, BPCGEN_INDENT_REF);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS:
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM:
				break;
			default:
				g_assert_not_reached();
		}
	}
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputs("};", fs);
	// write all msg name list
	write_msg_list(fs, msg_ls, BPCGEN_INDENT_REF);
	// end of testbench code. write macro
	BPCGEN_INDENT_PRINT;
	fputs("#endif", fs);

	// free msg list
	bpcgen_destructor_msg_list(msg_ls);

	// namespace over
	BPCGEN_INDENT_DEC;
	BPCGEN_INDENT_PRINT;
	fputc('}', fs);

}
