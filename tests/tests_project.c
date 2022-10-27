#include "main.h"
#include "../src/bpc_cmd.h"
#include "../src/bpc_fs.h"
#include "../src/bpc_encoding.h"
#include "../src/bpc_semantic_values.h"
#include "../src/y.tab.h"

// ===== Test Helper Functions =====

static BPCSMTV_PROTOCOL_BODY* check_has_identifier(const char* name) {
	BPCSMTV_PROTOCOL_BODY* identifier = bpcsmtv_registery_identifier_get(name);
	g_assert_nonnull(identifier);
	return identifier;
}
static void check_no_identifier(const char* name) {
	g_assert_false(bpcsmtv_registery_identifier_test(name));
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

	GSList* cursor = NULL;
	BPCSMTV_ENUM_MEMBER* data = NULL;
	for (cursor = menum->enum_body; cursor != NULL; cursor = cursor->next) {
		data = (BPCSMTV_ENUM_MEMBER*)cursor->data;
		g_assert_nonnull(data);
		if (g_str_equal(data->enum_member_name, member_name)) {
			break;
		}
	}

	g_assert_null(cursor);
}
static BPCSMTV_ENUM_MEMBER* check_enum_member(const char* name, const char* member_name) {
	BPCSMTV_ENUM* menum = check_enum(name);
	g_assert_nonnull(menum->enum_body);
	
	GSList* cursor = NULL;
	BPCSMTV_ENUM_MEMBER* data = NULL;
	for (cursor = menum->enum_body; cursor != NULL; cursor = cursor->next) {
		data = (BPCSMTV_ENUM_MEMBER*)cursor->data;
		g_assert_nonnull(data);
		if (g_str_equal(data->enum_member_name, member_name)) {
			break;
		}
	}
	
	g_assert_nonnull(cursor);
	return data;
}
static void check_enum_member_uvalue(const char* name, const char* member_name, uint64_t val) {
	BPCSMTV_ENUM_MEMBER* member = check_enum_member(name, member_name);
	g_assert_true(member->distributed_value_is_uint);
	g_assert_cmpuint(member->distributed_value.value_uint, ==, val);
}
static void check_enum_member_ivalue(const char* name, const char* member_name, int64_t val) {
	BPCSMTV_ENUM_MEMBER* member = check_enum_member(name, member_name);
	g_assert_false(member->distributed_value_is_uint);
	g_assert_cmpint(member->distributed_value.value_int, ==, val);
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
	g_assert_cmpuint(mstruct->struct_modifier->struct_size, ==, num_size);
	g_assert_cmpuint(mstruct->struct_modifier->struct_unit_size, ==, num_unit_size);
}
static BPCSMTV_VARIABLE* check_struct_no_variable(const char* name, const char* variable_name) {
	BPCSMTV_STRUCT* mstruct = check_struct(name);

	GSList* cursor;
	BPCSMTV_VARIABLE* data = NULL;
	for (cursor = mstruct->struct_body; cursor != NULL; cursor = cursor->next) {
		data = (BPCSMTV_VARIABLE*)cursor->data;
		g_assert_nonnull(data);
		if (g_str_equal(data->variable_name, variable_name)) {
			break;
		}
	}

	g_assert_null(cursor);
}
static BPCSMTV_VARIABLE* check_struct_variable(const char* name, const char* variable_name) {
	BPCSMTV_STRUCT* mstruct = check_struct(name);
	
	GSList* cursor;
	BPCSMTV_VARIABLE* data =NULL;
	for (cursor = mstruct->struct_body; cursor != NULL; cursor = cursor->next) {
		data = (BPCSMTV_VARIABLE*)cursor->data;
		g_assert_nonnull(data);
		if (g_str_equal(data->variable_name, variable_name)) {
			break;
		}
	}
	
	g_assert_nonnull(cursor);
	return data;
}
static void check_struct_variable_align(const char* name, const char* variable_name, uint32_t align) {
	BPCSMTV_VARIABLE* variable = check_struct_variable(name, variable_name);
	g_assert_nonnull(variable->variable_align);
	
	if (align == UINT32_C(0)) {
		g_assert_false(variable->variable_align->use_align);
	} else {
		g_assert_true(variable->variable_align->use_align);
		g_assert_cmpuint(variable->variable_align->padding_size, ==, align);
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
	g_assert_cmpuint(variable->variable_array->static_array_len, ==, tuple_len);
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
static void check_msg_index(const char* name, uint32_t idx) {
	BPCSMTV_MSG* msg = check_msg(name);
	g_assert_cmpuint(msg->msg_index, ==, idx);
}

// ===== Real Test Functions =====

static void project_test_core(BPCSMTV_DOCUMENT* document) {
	
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
	run_compiler(&fake_args, project_test_core);

	// free fake args
	fclose(fake_args.input_file);
}

void bpctest_project() {
	g_test_add_func("/project", project_test_wrapper);
}
