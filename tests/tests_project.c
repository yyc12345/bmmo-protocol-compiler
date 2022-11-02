#include "main.h"
#include "../src/bpc_cmd.h"
#include "../src/bpc_fs.h"
#include "../src/bpc_encoding.h"
#include "../src/bpc_semantic_values.h"
#include "../src/y.tab.h"

/*
Some test functions are not effective but it is enough 
just for a test.
*/

static BPCSMTV_DOCUMENT* test_document = NULL;

// ===== Test Helper Functions =====

static BPCSMTV_PROTOCOL_BODY* find_identifier_from_gslist(const char* name, GSList* ls) {
	GSList* cursor;
	BPCSMTV_PROTOCOL_BODY* data = NULL;
	for (cursor = ls; cursor != NULL; cursor = cursor->next) {
		data = (BPCSMTV_PROTOCOL_BODY*)cursor->data;
		g_assert_nonnull(data);

		switch (data->node_type) {
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS:
				if (g_str_equal(data->node_data.alias_data->custom_type, name)) goto gotten_identifier;
				else break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM:
				if (g_str_equal(data->node_data.enum_data->enum_name, name)) goto gotten_identifier;
				else break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT:
				if (g_str_equal(data->node_data.struct_data->struct_name, name)) goto gotten_identifier;
				else break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG:
				if (g_str_equal(data->node_data.msg_data->msg_name, name)) goto gotten_identifier;
				else break;
			default:
				g_assert_not_reached();
		}
	}

gotten_identifier:
	return cursor == NULL ? NULL : data;
}
static BPCSMTV_VARIABLE* find_variable_from_gslist(const char* name, GSList* ls) {
	GSList* cursor;
	BPCSMTV_VARIABLE* data = NULL;
	for (cursor = ls; cursor != NULL; cursor = cursor->next) {
		data = (BPCSMTV_VARIABLE*)cursor->data;
		g_assert_nonnull(data);
		if (g_str_equal(data->variable_name, name)) {
			break;
		}
	}

	return cursor == NULL ? NULL : data;
}
static BPCSMTV_ENUM_MEMBER* find_member_from_gslist(const char* name, GSList* ls) {
	GSList* cursor = NULL;
	BPCSMTV_ENUM_MEMBER* data = NULL;
	for (cursor = ls; cursor != NULL; cursor = cursor->next) {
		data = (BPCSMTV_ENUM_MEMBER*)cursor->data;
		g_assert_nonnull(data);
		if (g_str_equal(data->enum_member_name, name)) {
			break;
		}
	}

	return cursor == NULL ? NULL : data;
}

static BPCSMTV_PROTOCOL_BODY* check_has_identifier(const char* name) {
	g_assert_nonnull(test_document);
	g_assert_nonnull(test_document->protocol_body);

	BPCSMTV_PROTOCOL_BODY* identifier = find_identifier_from_gslist(name, test_document->protocol_body);
	g_assert_nonnull(identifier);
	return identifier;
}
static void check_no_identifier(const char* name) {
	g_assert_nonnull(test_document);
	if (test_document->protocol_body == NULL) return;

	BPCSMTV_PROTOCOL_BODY* identifier = find_identifier_from_gslist(name, test_document->protocol_body);
	g_assert_null(identifier);
}

static BPCSMTV_ALIAS* check_alias(const char* name) {
	BPCSMTV_PROTOCOL_BODY* identifier = check_has_identifier(name);
	g_assert_true(identifier->node_type == BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS);
	g_assert_nonnull(identifier->node_data.alias_data);
	return identifier->node_data.alias_data;
}
static BPCSMTV_ENUM* check_enum(const char* name) {
	BPCSMTV_PROTOCOL_BODY* identifier = check_has_identifier(name);
	g_assert_true(identifier->node_type == BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM);
	g_assert_nonnull(identifier->node_data.enum_data);
	return identifier->node_data.enum_data;
}
static BPCSMTV_STRUCT* check_struct(const char* name) {
	BPCSMTV_PROTOCOL_BODY* identifier = check_has_identifier(name);
	g_assert_true(identifier->node_type == BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT);
	g_assert_nonnull(identifier->node_data.struct_data);
	return identifier->node_data.struct_data;
}
static BPCSMTV_MSG* check_msg(const char* name) {
	BPCSMTV_PROTOCOL_BODY* identifier = check_has_identifier(name);
	g_assert_true(identifier->node_type == BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG);
	g_assert_nonnull(identifier->node_data.msg_data);
	return identifier->node_data.msg_data;
}

static void check_alias_bt(const char* name, BPCSMTV_BASIC_TYPE bt) {
	BPCSMTV_ALIAS* alias = check_alias(name);
	g_assert_true(alias->basic_type == bt);
}

static void check_enum_no_member(const char* name, const char* member_name) {
	BPCSMTV_ENUM* menum = check_enum(name);
	g_assert_nonnull(menum->enum_body);

	BPCSMTV_ENUM_MEMBER* data = find_member_from_gslist(member_name, menum->enum_body);
	g_assert_null(data);
}
static BPCSMTV_ENUM_MEMBER* check_enum_member(const char* name, const char* member_name) {
	BPCSMTV_ENUM* menum = check_enum(name);
	g_assert_nonnull(menum->enum_body);

	BPCSMTV_ENUM_MEMBER* data = find_member_from_gslist(member_name, menum->enum_body);
	g_assert_nonnull(data);
	return data;
}
static void check_enum_member_uvalue(const char* name, const char* member_name, uint64_t val) {
	BPCSMTV_ENUM_MEMBER* member = check_enum_member(name, member_name);
	g_assert_true(member->distributed_value_is_uint);
	g_assert_cmpuint(member->distributed_value.value_uint, == , val);
}
static void check_enum_member_ivalue(const char* name, const char* member_name, int64_t val) {
	BPCSMTV_ENUM_MEMBER* member = check_enum_member(name, member_name);
	g_assert_false(member->distributed_value_is_uint);
	g_assert_cmpint(member->distributed_value.value_int, == , val);
}

static void check_struct_modifier(const char* name, bool is_narrow) {
	BPCSMTV_STRUCT* mstruct = check_struct(name);
	g_assert_nonnull(mstruct->struct_modifier);
	g_assert_true(mstruct->struct_modifier->has_set_field_layout);
	g_assert_true(mstruct->struct_modifier->is_narrow == is_narrow);
}
static void check_struct_natural_data(const char* name, uint32_t num_size, uint32_t num_unit_size) {
	BPCSMTV_STRUCT* mstruct = check_struct(name);
	g_assert_nonnull(mstruct->struct_modifier);

	g_assert_true(mstruct->struct_modifier->has_set_field_layout);
	g_assert_false(mstruct->struct_modifier->is_narrow);
	g_assert_cmpuint(mstruct->struct_modifier->struct_size, == , num_size);
	g_assert_cmpuint(mstruct->struct_modifier->struct_unit_size, == , num_unit_size);
}
static void check_struct_no_variable(const char* name, const char* variable_name) {
	BPCSMTV_STRUCT* mstruct = check_struct(name);

	BPCSMTV_VARIABLE* data = find_variable_from_gslist(variable_name, mstruct->struct_body);
	g_assert_null(data);
}
static BPCSMTV_VARIABLE* check_struct_variable(const char* name, const char* variable_name) {
	BPCSMTV_STRUCT* mstruct = check_struct(name);

	BPCSMTV_VARIABLE* data = find_variable_from_gslist(variable_name, mstruct->struct_body);
	g_assert_nonnull(data);
	return data;
}
static void check_struct_variable_align(const char* name, const char* variable_name, uint32_t align) {
	BPCSMTV_VARIABLE* variable = check_struct_variable(name, variable_name);
	g_assert_nonnull(variable->variable_align);

	if (align == UINT32_C(0)) {
		g_assert_false(variable->variable_align->use_align);
	} else {
		g_assert_true(variable->variable_align->use_align);
		g_assert_cmpuint(variable->variable_align->padding_size, == , align);
	}
}
static void check_struct_variable_no_array(const char* name, const char* variable_name) {
	BPCSMTV_VARIABLE* variable = check_struct_variable(name, variable_name);
	g_assert_nonnull(variable->variable_array);

	g_assert_false(variable->variable_array->is_array);
}
static void check_struct_variable_tuple(const char* name, const char* variable_name, uint32_t tuple_len) {
	BPCSMTV_VARIABLE* variable = check_struct_variable(name, variable_name);
	g_assert_nonnull(variable->variable_array);

	g_assert_true(variable->variable_array->is_array);
	g_assert_true(variable->variable_array->is_static_array);
	g_assert_cmpuint(variable->variable_array->static_array_len, == , tuple_len);
}
static void check_struct_variable_list(const char* name, const char* variable_name) {
	BPCSMTV_VARIABLE* variable = check_struct_variable(name, variable_name);
	g_assert_nonnull(variable->variable_array);

	g_assert_true(variable->variable_array->is_array);
	g_assert_false(variable->variable_array->is_static_array);
}

static void check_msg_modifier(const char* name, bool is_narrow, bool is_reliable) {
	BPCSMTV_MSG* msg = check_msg(name);
	g_assert_nonnull(msg->msg_modifier);

	g_assert_true(msg->msg_modifier->has_set_reliability);
	g_assert_true(msg->msg_modifier->is_reliable == is_reliable);
	g_assert_true(msg->msg_modifier->has_set_field_layout);
	g_assert_true(msg->msg_modifier->is_narrow == is_narrow);
}
static void check_msg_no_variable(const char* name, const char* variable_name) {
	BPCSMTV_MSG* msg = check_msg(name);

	BPCSMTV_VARIABLE* data = find_variable_from_gslist(variable_name, msg->msg_body);
	g_assert_null(data);
}
static BPCSMTV_VARIABLE* check_msg_variable(const char* name, const char* variable_name) {
	BPCSMTV_MSG* msg = check_msg(name);

	BPCSMTV_VARIABLE* data = find_variable_from_gslist(variable_name, msg->msg_body);
	g_assert_nonnull(data);
	return data;
}
static void check_msg_index(const char* name, uint32_t idx) {
	BPCSMTV_MSG* msg = check_msg(name);
	g_assert_cmpuint(msg->msg_index, == , idx);
}

// ===== Real Test Functions =====

static void project_test_core(BPCSMTV_DOCUMENT* document) {
	// setup document
	test_document = document;

	//  === Alias ===
	check_alias_bt("tests_alias_int", BPCSMTV_BASIC_TYPE_UINT32);
	check_alias_bt("tests_alias_float", BPCSMTV_BASIC_TYPE_FLOAT);
	check_alias_bt("tests_alias_string", BPCSMTV_BASIC_TYPE_STRING);
	check_alias("tests_alias_dup_error");

	// === Enum ===
	check_enum_member_uvalue("tests_enum", "entry", UINT64_C(0));
	check_enum_member_ivalue("tests_enum_negative", "entry2", INT64_C(-114513));
	check_enum_member_uvalue("tests_enum_limit_uint", "entry3", UINT64_C(0));
	check_enum_member_ivalue("tests_enum_limit_int", "entry3", INT64_C(-9223372036854775807) - INT64_C(1));
	check_enum_member_ivalue("tests_enum_limit_int8", "entry3", INT64_C(-128));

	check_no_identifier("tests_enum_error_overflow");
	check_no_identifier("tests_enum_error_raw_overflow");
	check_no_identifier("tests_enum_error_dup_member");
	check_no_identifier("tests_enum_error_type");

	// === Struct ===
	check_struct("tests_struct_blank");
	check_struct_modifier("tests_struct", true);
	
	check_struct_variable("tests_struct_successive_decl", "data1");
	check_struct_variable("tests_struct_successive_decl", "data2");
	check_struct_variable("tests_struct_successive_decl", "data3");
	check_struct_variable_list("tests_struct_successive_decl", "data4");
	check_struct_variable_list("tests_struct_successive_decl", "data5");
	check_struct_variable_list("tests_struct_successive_decl", "data6");
	check_struct_variable_tuple("tests_struct_successive_decl", "data7", UINT32_C(2));
	check_struct_variable_tuple("tests_struct_successive_decl", "data8", UINT32_C(2));
	check_struct_variable_tuple("tests_struct_successive_decl", "data9", UINT32_C(2));
	check_struct_variable_align("tests_struct_successive_decl", "data10", UINT32_C(2));
	check_struct_variable_align("tests_struct_successive_decl", "data11", UINT32_C(2));
	check_struct_variable_align("tests_struct_successive_decl", "data12", UINT32_C(2));

	check_struct_variable_list("tests_struct_list", "data1");
	check_struct_variable_list("tests_struct_list", "data9");
	check_struct_variable_tuple("tests_struct_tuple", "data1", UINT32_C(8));
	check_struct_variable_tuple("tests_struct_tuple", "data9", UINT32_C(8));
	check_struct_variable_align("tests_struct_align", "data1", UINT32_C(4));
	check_struct_variable_align("tests_struct_align", "data9", UINT32_C(4));

	check_struct_variable_no_array("tests_struct_hybrid", "data7");
	check_struct_variable_align("tests_struct_hybrid", "data7", UINT32_C(0));
	check_struct_variable_no_array("tests_struct_hybrid", "data8");
	check_struct_variable_align("tests_struct_hybrid", "data8", UINT32_C(0));
	check_struct_variable_no_array("tests_struct_hybrid", "data9");
	check_struct_variable_align("tests_struct_hybrid", "data9", UINT32_C(0));

	check_struct_modifier("tests_struct_natural1", false);
	check_struct_modifier("tests_struct_natural2", false);

	check_struct_variable_align("tests_struct_natural_align1", "data1", UINT32_C(1));
	check_struct_variable_align("tests_struct_natural_align1", "data2", UINT32_C(0));
	check_struct_variable_align("tests_struct_natural_align1", "data3", UINT32_C(1));
	check_struct_variable_align("tests_struct_natural_align1", "data4", UINT32_C(0));
	check_struct_natural_data("tests_struct_natural_align1", UINT32_C(10), UINT32_C(2));
	check_struct_variable_align("tests_struct_natural_align2", "data1", UINT32_C(3));
	check_struct_variable_align("tests_struct_natural_align2", "data2", UINT32_C(0));
	check_struct_variable_align("tests_struct_natural_align2", "data3", UINT32_C(1));
	check_struct_variable_align("tests_struct_natural_align2", "data4", UINT32_C(0));
	check_struct_variable_align("tests_struct_natural_align2", "data5", UINT32_C(2));
	check_struct_variable_align("tests_struct_natural_align2", "data6", UINT32_C(2));
	check_struct_natural_data("tests_struct_natural_align2", UINT32_C(64), UINT32_C(4));

	check_struct_modifier("tests_struct_nospec_natural1", false);
	check_struct_modifier("tests_struct_nospec_natural2", false);
	check_struct_modifier("tests_struct_nospec_narrow1", true);
	check_struct_modifier("tests_struct_nospec_narrow2", true);
	check_struct_modifier("tests_struct_nospec_narrow3", true);
	check_struct_modifier("tests_struct_nospec_narrow4", true);

	check_struct_variable_align("tests_struct_natural_warning", "data1", UINT32_C(0));

	check_struct_variable("tests_struct_error_dup_variable", "data1");
	check_struct_variable("tests_struct_error_dup_variable", "data2");

	check_struct_variable_align("tests_struct_error_overflow", "data1", UINT32_C(0));
	check_struct_variable("tests_struct_error_overflow", "data2");
	check_struct_variable_no_array("tests_struct_error_overflow", "data3");
	check_struct_variable("tests_struct_error_overflow", "data4");

	check_struct_no_variable("tests_struct_error_unknow_type", "data1");
	check_struct_variable("tests_struct_error_unknow_type", "data2");

	check_no_identifier("tests_struct_error_dup_modifier");

	check_struct_no_variable("tests_struct_error_dup_array", "data1");
	check_struct_variable("tests_struct_error_dup_array", "data2");

	// === Msg ===
	check_msg_modifier("tests_msg1", true, true);
	check_msg_index("tests_msg1", UINT32_C(0));
	check_msg_modifier("tests_msg2", false, false);
	check_msg_index("tests_msg2", UINT32_C(1));
	check_msg_modifier("tests_nospec_msg", false, true);
	check_msg_index("tests_nospec_msg", UINT32_C(2));

	check_no_identifier("tests_msg_error_dup_modifier");

	check_msg_no_variable("tests_msg_error_invalid_type", "data1");
	check_msg_variable("tests_msg_error_invalid_type", "data2");
	check_msg_index("tests_msg_error_invalid_type", UINT32_C(3));

	// make test doc invalid
	test_document = NULL;
}

static void project_test_wrapper(void) {
	// construct a fake args
	BPCCMD_PARSED_ARGS fake_args;

#ifdef G_OS_WIN32
	static const char* test_filename = "examples\\test.bp";
#else
	static const char* test_filename = "examples/test.bp";
#endif

	gchar* glibfs_infile = bpcenc_utf8_to_glibfs(test_filename);
	fake_args.input_file = bpcfs_fopen_glibfs(glibfs_infile, true);
	g_assert(fake_args.input_file != NULL);
	g_free(glibfs_infile);

	fake_args.out_cpp_header_file = fake_args.out_cpp_source_file =
		fake_args.out_csharp_file = fake_args.out_proto_file = fake_args.out_python_file = NULL;
	fake_args.ref_cpp_relative_hdr = NULL;

	// call test
	bpcyy_run_compiler(&fake_args, project_test_core);

	// free fake args
	fclose(fake_args.input_file);
}

void bpctest_project() {
	g_test_add_func("/project", project_test_wrapper);
}
