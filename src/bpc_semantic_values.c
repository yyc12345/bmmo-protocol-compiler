#include "bpc_semantic_values.h"
#include "bpc_error.h"
#include <inttypes.h>

/// <summary>
/// item is `char*` of each variables name
/// </summary>
static GHashTable* hashtable_variables = NULL;
/// <summary>
/// item is `BPCSMTV_REGISTERY_IDENTIFIER_ITEM*` of each identifier
/// </summary>
static GHashTable* hashtable_identifier = NULL;
static uint32_t msg_index_distributor = 0;

static const char* basic_type_showcase[] = {
	"float", "double",
	"int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64",
	"string"
};
static const size_t basic_type_len = sizeof(basic_type_showcase) / sizeof(char*);
static const uint32_t basic_type_sizeof[] = {
	4, 8,
	1, 2, 4, 8, 1, 2, 4, 8,
	0
};

static const uint64_t uint_max_limit[] = { UINT8_MAX, UINT16_MAX, UINT32_MAX, UINT64_MAX };
static const int64_t int_max_limit[] = { INT8_MAX, INT16_MAX, INT32_MAX, INT64_MAX };
static const int64_t int_min_limit[] = { INT8_MIN, INT16_MIN, INT32_MIN, INT64_MIN };

// ==================== Parse Functions ====================

BPCSMTV_BASIC_TYPE bpcsmtv_parse_basic_type(const char* strl) {
	for (size_t i = 0; i < basic_type_len; ++i) {
		if (g_str_equal(strl, basic_type_showcase[i])) return (BPCSMTV_BASIC_TYPE)i;
	}

	// error
	return BPCSMTV_BASIC_TYPE_INVALID;
}
bool bpcsmtv_parse_reliability(const char* strl) {
	return g_str_equal(strl, "reliable");
}
bool bpcsmtv_parse_field_layout(const char* strl) {
	return g_str_equal(strl, "narrow");
}
bool bpcsmtv_parse_number(const char* strl, size_t len, size_t start_margin, size_t end_margin, BPCSMTV_COMPOUND_NUMBER* result) {
	// setup result first
	result->num_int = 0i64;
	result->num_uint = 0ui64;
	result->success_int = false;
	result->success_uint = false;

	// check length
	if (len == 0u) return false;

	// setup basic value
	// construct valid number string
	char* copiedstr = (char*)g_memdup2(strl, len + 1);
	char* start = copiedstr, * end = copiedstr + len - 1u;
	char* start_border = start + start_margin, * end_border = end - end_margin;
	char* start_cursor = start, * end_cursor = end;

	// fill padding area with blank
	for (start_cursor = start; start_cursor <= end && start_cursor < start_border; ++start_cursor) *start_cursor = '\0';
	for (end_cursor = end; end_cursor >= start && end_cursor > end_border; --end_cursor) *end_cursor = '\0';
	// move to border
	// then do essential padding and whitespace detection.
	// end_cursor only need move to start_cursor because all previous chars has been set as NULL.
#define IS_LEGAL_BLANK(v) ((v)==' '||(v)=='\0'||(v)=='\t')
	for (start_cursor = start_border; start_cursor <= end && IS_LEGAL_BLANK(*start_cursor); ++start_cursor) *start_cursor = '\0';
	for (end_cursor = end_border; end_cursor >= start_cursor && IS_LEGAL_BLANK(*end_cursor); --end_cursor) *end_cursor = '\0';
#undef IS_LEGAL_BLANK

	// move out of range or tail lower than head
	if (start_cursor > end || end_cursor < start_cursor) {
		// invalid
		g_free(copiedstr);
		return false;
	} else {
		// try parse
		result->success_int = g_ascii_string_to_signed(start_cursor, 10u, INT64_MIN, INT64_MAX, &(result->num_int), NULL);
		result->success_uint = g_ascii_string_to_unsigned(start_cursor, 10u, 0ui64, UINT64_MAX, &(result->num_uint), NULL);
		g_free(copiedstr);
		return result->success_int || result->success_uint;
	}
}

// ==================== Constructor/Deconstructor Functions ====================

BPCSMTV_VARIABLE_ARRAY* bpcsmtv_constructor_variable_array() {
	return g_new0(BPCSMTV_VARIABLE_ARRAY, 1);
}
BPCSMTV_VARIABLE_ARRAY* bpcsmtv_duplicator_variable_array(BPCSMTV_VARIABLE_ARRAY* data) {
	// all basic types, do shallow copy
	return g_memdup2(data, sizeof(BPCSMTV_VARIABLE_ARRAY));
}
void bpcsmtv_destructor_variable_array(BPCSMTV_VARIABLE_ARRAY* data) {
	g_free(data);
}

BPCSMTV_VARIABLE_TYPE* bpcsmtv_constructor_variable_type() {
	return g_new0(BPCSMTV_VARIABLE_TYPE, 1);
}
BPCSMTV_VARIABLE_TYPE* bpcsmtv_duplicator_variable_type(BPCSMTV_VARIABLE_TYPE* data) {
	// do a deep copy
	BPCSMTV_VARIABLE_TYPE* ndata = g_new0(BPCSMTV_VARIABLE_TYPE, 1);
	// basic
	ndata->is_basic_type = data->is_basic_type;
	if (ndata->is_basic_type) {
		ndata->type_data.basic_type = data->type_data.basic_type;
	} else {
		ndata->type_data.custom_type = g_strdup(data->type_data.custom_type);
	}
	// underlaying
	ndata->full_uncover_is_basic_type = data->full_uncover_is_basic_type;
	ndata->semi_uncover_is_basic_type = data->semi_uncover_is_basic_type;
	ndata->full_uncover_basic_type = data->full_uncover_basic_type;
	if (!data->semi_uncover_is_basic_type) {
		ndata->semi_uncover_custom_type = g_strdup(data->semi_uncover_custom_type);
	}

	return ndata;
}
void bpcsmtv_destructor_variable_type(BPCSMTV_VARIABLE_TYPE* data) {
	if (data == NULL) return;

	if (!data->is_basic_type) {
		bpcsmtv_destructor_string(data->type_data.custom_type);
	}
	if (!data->semi_uncover_is_basic_type) {
		bpcsmtv_destructor_string(data->semi_uncover_custom_type);
	}

	g_free(data);
}

BPCSMTV_VARIABLE_ALIGN* bpcsmtv_constructor_variable_align() {
	return g_new0(BPCSMTV_VARIABLE_ALIGN, 1);
}
BPCSMTV_VARIABLE_ALIGN* bpcsmtv_duplicator_variable_align(BPCSMTV_VARIABLE_ALIGN* data) {
	// all basic types, do shallow copy
	return g_memdup2(data, sizeof(BPCSMTV_VARIABLE_ALIGN));
}
void bpcsmtv_destructor_variable_align(BPCSMTV_VARIABLE_ALIGN* data) {
	g_free(data);
}

// ====================

BPCSMTV_ALIAS* bpcsmtv_constructor_alias() {
	return g_new0(BPCSMTV_ALIAS, 1);
}
void bpcsmtv_destructor_alias(BPCSMTV_ALIAS* data) {
	if (data == NULL) return;

	BPCSMTV_ALIAS* rdata = (BPCSMTV_ALIAS*)data;
	bpcsmtv_destructor_string(rdata->custom_type);

	g_free(rdata);
}

BPCSMTV_ENUM* bpcsmtv_constructor_enum() {
	return g_new0(BPCSMTV_ENUM, 1);
}
void bpcsmtv_destructor_enum(BPCSMTV_ENUM* data) {
	if (data == NULL) return;

	BPCSMTV_ENUM* rdata = (BPCSMTV_ENUM*)data;
	bpcsmtv_destructor_string(rdata->enum_name);
	bpcsmtv_destructor_slist_enum_member(rdata->enum_body);

	g_free(rdata);
}

BPCSMTV_STRUCT* bpcsmtv_constructor_struct() {
	return g_new0(BPCSMTV_STRUCT, 1);
}
void bpcsmtv_destructor_struct(BPCSMTV_STRUCT* data) {
	if (data == NULL) return;

	BPCSMTV_STRUCT* rdata = (BPCSMTV_STRUCT*)data;
	bpcsmtv_destructor_string(rdata->struct_name);
	bpcsmtv_destructor_struct_modifier(rdata->struct_modifier);
	bpcsmtv_destructor_slist_variable(rdata->struct_body);

	g_free(rdata);
}

BPCSMTV_MSG* bpcsmtv_constructor_msg() {
	return g_new0(BPCSMTV_MSG, 1);
}
void bpcsmtv_destructor_msg(BPCSMTV_MSG* data) {
	if (data == NULL) return;

	BPCSMTV_MSG* rdata = (BPCSMTV_MSG*)data;
	bpcsmtv_destructor_string(rdata->msg_name);
	bpcsmtv_destructor_struct_modifier(rdata->msg_modifier);
	bpcsmtv_destructor_slist_variable(rdata->msg_body);

	g_free(rdata);
}

// ====================

BPCSMTV_STRUCT_MODIFIER* bpcsmtv_constructor_struct_modifier() {
	return g_new0(BPCSMTV_STRUCT_MODIFIER, 1);
}
void bpcsmtv_destructor_struct_modifier(BPCSMTV_STRUCT_MODIFIER* data) {
	g_free(data);
}

BPCSMTV_VARIABLE* bpcsmtv_constructor_variable() {
	return g_new0(BPCSMTV_VARIABLE, 1);
}
void bpcsmtv_destructor_variable(gpointer data) {
	if (data == NULL) return;

	BPCSMTV_VARIABLE* rdata = (BPCSMTV_VARIABLE*)data;
	bpcsmtv_destructor_string(rdata->variable_name);
	bpcsmtv_destructor_variable_type(rdata->variable_type);
	bpcsmtv_destructor_variable_array(rdata->variable_array);
	bpcsmtv_destructor_variable_align(rdata->variable_align);

	g_free(rdata);
}

BPCSMTV_ENUM_MEMBER* bpcsmtv_constructor_enum_member() {
	return g_new0(BPCSMTV_ENUM_MEMBER, 1);
}
void bpcsmtv_destructor_enum_member(gpointer data) {
	if (data == NULL) return;

	BPCSMTV_ENUM_MEMBER* rdata = (BPCSMTV_ENUM_MEMBER*)data;
	bpcsmtv_destructor_string(rdata->enum_member_name);

	g_free(rdata);
}

BPCSMTV_PROTOCOL_BODY* bpcsmtv_constructor_protocol_body() {
	return g_new0(BPCSMTV_PROTOCOL_BODY, 1);
}
void bpcsmtv_destructor_protocol_body(gpointer data) {
	if (data == NULL) return;

	BPCSMTV_PROTOCOL_BODY* rdata = (BPCSMTV_PROTOCOL_BODY*)data;
	switch (rdata->node_type) {
		case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS:
			bpcsmtv_destructor_alias(rdata->node_data.alias_data);
			break;
		case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM:
			bpcsmtv_destructor_enum(rdata->node_data.enum_data);
			break;
		case BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT:
			bpcsmtv_destructor_struct(rdata->node_data.struct_data);
			break;
		case BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG:
			bpcsmtv_destructor_msg(rdata->node_data.msg_data);
			break;
	}

	g_free(rdata);
}

BPCSMTV_DOCUMENT* bpcsmtv_constructor_document() {
	return g_new0(BPCSMTV_DOCUMENT, 1);
}
void bpcsmtv_destructor_document(BPCSMTV_DOCUMENT* data) {
	if (data == NULL) return;

	BPCSMTV_DOCUMENT* rdata = (BPCSMTV_DOCUMENT*)data;
	bpcsmtv_destructor_slist_string(rdata->namespace_data);
	bpcsmtv_destructor_slist_protocol_body(rdata->protocol_body);

	g_free(rdata);
}

// ====================

void bpcsmtv_destructor_string(gpointer data) {
	g_free(data);
}
void bpcsmtv_destructor_slist_string(GSList* data) {
	if (data == NULL) return;
	g_slist_free_full(data, bpcsmtv_destructor_string);
}
void bpcsmtv_destructor_slist_protocol_body(GSList* data) {
	if (data == NULL) return;
	g_slist_free_full(data, bpcsmtv_destructor_protocol_body);
}
void bpcsmtv_destructor_slist_enum_member(GSList* data) {
	if (data == NULL) return;
	g_slist_free_full(data, bpcsmtv_destructor_enum_member);
}
void bpcsmtv_destructor_slist_variable(GSList* data) {
	if (data == NULL) return;
	g_slist_free_full(data, bpcsmtv_destructor_variable);
}

// ==================== Registery Functions ====================
void bpcsmtv_registery_variables_reset() {
	// init hashtable when necessary
	if (hashtable_variables == NULL) {
		// we use it as Set, so we only need to free key, otherwise, string will be freed twice.
		hashtable_variables = g_hash_table_new_full(g_str_hash, g_str_equal, bpcsmtv_destructor_string, NULL);
	} else {
		// clear hashtable
		g_hash_table_remove_all(hashtable_variables);
	}
}

bool bpcsmtv_registery_variables_test(const char* name) {
	return g_hash_table_lookup_extended(hashtable_variables, name, NULL, NULL);
}

void bpcsmtv_registery_variables_add(const char* name) {
	g_hash_table_add(hashtable_variables, g_strdup(name));
}



void bpcsmtv_registery_identifier_reset() {
	// init hashtable when necessary
	if (hashtable_identifier == NULL) {
		// hashtable is just a search method, so we do not need free values
		hashtable_identifier = g_hash_table_new_full(g_str_hash, g_str_equal, bpcsmtv_destructor_string, NULL);
	} else {
		// clear hashtable
		g_hash_table_remove_all(hashtable_identifier);
	}

	// reset message index distributor
	msg_index_distributor = 0u;
}

bool bpcsmtv_registery_identifier_test(const char* name) {
	return g_hash_table_lookup_extended(hashtable_identifier, name, NULL, NULL);
}

BPCSMTV_PROTOCOL_BODY* bpcsmtv_registery_identifier_get(const char* name) {
	return g_hash_table_lookup(hashtable_identifier, name);
}

uint32_t bpcsmtv_registery_identifier_distribute_index() {
	return msg_index_distributor++;
}

void bpcsmtv_registery_identifier_add(BPCSMTV_PROTOCOL_BODY* data) {
	char* identifier_name = NULL;
	switch (data->node_type) {
		case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS:
			identifier_name = data->node_data.alias_data->custom_type;
			break;
		case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM:
			identifier_name = data->node_data.enum_data->enum_name;
			break;
		case BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT:
			identifier_name = data->node_data.struct_data->struct_name;
			break;
		case BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG:
			identifier_name = data->node_data.msg_data->msg_name;
			break;
	}

	// add for hashtable
	g_hash_table_replace(hashtable_identifier, g_strdup(identifier_name), data);
}

// ==================== Utils Functions ====================

bool bpcsmtv_get_offset_number(BPCSMTV_COMPOUND_NUMBER* num, uint32_t* outnum) {
	if (outnum != NULL) *outnum = 0ui32;

	if (!num->success_uint) return false;
	if (num->num_uint > UINT32_MAX) return false;

	if (outnum != NULL) *outnum = (uint32_t)outnum;
	return true;
}

bool bpcsmtv_is_basic_type_suit_for_enum(BPCSMTV_BASIC_TYPE bt) {
	switch (bt) {
		case BPCSMTV_BASIC_TYPE_INT8:
		case BPCSMTV_BASIC_TYPE_INT16:
		case BPCSMTV_BASIC_TYPE_INT32:
		case BPCSMTV_BASIC_TYPE_UINT8:
		case BPCSMTV_BASIC_TYPE_UINT16:
		case BPCSMTV_BASIC_TYPE_UINT32:
			return true;
		case BPCSMTV_BASIC_TYPE_INT64:
		case BPCSMTV_BASIC_TYPE_UINT64:
		case BPCSMTV_BASIC_TYPE_FLOAT:
		case BPCSMTV_BASIC_TYPE_DOUBLE:
		case BPCSMTV_BASIC_TYPE_STRING:
		default:
			return false;
	}
}

bool bpcsmtv_is_modifier_suit_struct(BPCSMTV_STRUCT_MODIFIER* modifier) {
	return (!modifier->has_set_reliability);
}

void bpcsmtv_setup_field_layout(GSList* variables, BPCSMTV_STRUCT_MODIFIER* modifier) {
	// check data
	bool can_be_natural = true;
	GSList* cursor;
	BPCSMTV_VARIABLE* variable;
	for (cursor = variables; cursor != NULL; cursor = cursor->next) {
		variable = (BPCSMTV_VARIABLE*)cursor->data;

		if (variable->variable_array->is_array && !variable->variable_array->is_static_array) {
			// dynamic array is not allowed
			can_be_natural = false;
			break;
		}
		if (variable->variable_type->full_uncover_is_basic_type) {
			// struct
			// look up whether it is natural struct
			BPCSMTV_PROTOCOL_BODY* entry = bpcsmtv_registery_identifier_get(variable->variable_type->type_data.custom_type);
			if (entry->node_data.struct_data->struct_modifier->is_narrow) {
				// narrow struct is not allowed
				can_be_natural = false;
				break;
			}
		} else {
			// enum or alias
			if (variable->variable_type->full_uncover_basic_type == BPCSMTV_BASIC_TYPE_STRING) {
				// string is not allowed
				can_be_natural = false;
				break;
			}
		}
	}

	// change modifier
	if (modifier->has_set_field_layout) {
		if (!can_be_natural && !modifier->is_narrow) {
			bpcerr_warning(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "your speficied field layout modifier `nature` is not suit for current struct. we change it to `narrow` forcely.");

			modifier->is_narrow = !can_be_natural;
		}
	} else {
		bpcerr_info(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "field layout modifier not found, we assume it as %s.",
			can_be_natural ? "`natural`" : "`narrow`");

		modifier->has_set_field_layout = true;
		modifier->is_narrow = !can_be_natural;
	}

	// set align prop
	if (modifier->is_narrow) {



		modifier->struct_size = 0u;
	} else {

		// todo: finish padding

		modifier->struct_size = 1u;
	}

}

void bpcsmtv_setup_reliability(BPCSMTV_STRUCT_MODIFIER* modifier) {
	if (!modifier->has_set_reliability) {
		bpcerr_info(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "realibility modifier not found, we assume it as `reliable`.");

		modifier->has_set_reliability = true;
		modifier->is_reliable = true;
	}
}

void bpcsmtv_analyse_underlaying_type(BPCSMTV_VARIABLE_TYPE* variables) {
	if (variables->is_basic_type) {
		variables->full_uncover_is_basic_type = true;
		variables->full_uncover_basic_type = variables->type_data.basic_type;
		variables->semi_uncover_is_basic_type = true;
		variables->semi_uncover_custom_type = NULL;
	} else {
		// check user defined type
		// token type will never be msg accoring to syntax define
		BPCSMTV_PROTOCOL_BODY* entry = bpcsmtv_registery_identifier_get(variables->type_data.custom_type);
		switch (entry->node_type) {
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS:
				variables->full_uncover_is_basic_type = true;
				variables->full_uncover_basic_type = entry->node_data.alias_data->basic_type;
				variables->semi_uncover_is_basic_type = true;
				variables->semi_uncover_custom_type = NULL;
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM:
				variables->full_uncover_is_basic_type = true;
				variables->full_uncover_basic_type = entry->node_data.enum_data->enum_basic_type;
				variables->semi_uncover_is_basic_type = false;
				variables->semi_uncover_custom_type = g_strdup(variables->type_data.custom_type);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT:
				variables->full_uncover_is_basic_type = false;
				variables->full_uncover_basic_type = BPCSMTV_BASIC_TYPE_INVALID;
				variables->semi_uncover_is_basic_type = false;
				variables->semi_uncover_custom_type = g_strdup(variables->type_data.custom_type);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG:
			default:
				break;	// skip
		}
	}
}

void bpcsmtv_assign_enum_member_value(BPCSMTV_ENUM_MEMBER* member, BPCSMTV_COMPOUND_NUMBER* number) {
	if (number == NULL) {
		member->have_specific_value = false;
	} else {
		member->have_specific_value = true;
		// copy data when data is valid
		member->specified_value.value_uint = number->success_uint ? number->num_uint : 0ui64;
		member->specified_value.value_int = number->success_int ? number->num_int : 0i64;
	}
}

void bpcsmtv_ensure_enum_member_value(GSList* parents, BPCSMTV_ENUM_MEMBER* member) {
	// convert all non-spec value to specified value
	if (!member->have_specific_value) {
		// try distribute one
		if (parents == NULL) {
			// no parents
			member->specified_value.value_uint = 0ui64;
			member->specified_value.value_int = 0i64;
		} else {
			member->specified_value.value_uint = ((BPCSMTV_ENUM_MEMBER*)parents->data)->specified_value.value_uint + 1ui64;
			member->specified_value.value_int = ((BPCSMTV_ENUM_MEMBER*)parents->data)->specified_value.value_int + 1i64;
		}

		// sign has distributed
		member->have_specific_value = true;
	}

}

void bpcsmtv_setup_enum_body_value_sign(GSList* enum_body, BPCSMTV_BASIC_TYPE bt) {
	// check sign
	bool is_positive = true;
	switch (bt) {
		case BPCSMTV_BASIC_TYPE_INT64:
		case BPCSMTV_BASIC_TYPE_INT32:
		case BPCSMTV_BASIC_TYPE_INT16:
		case BPCSMTV_BASIC_TYPE_INT8:
			is_positive = false;
			break;
		case BPCSMTV_BASIC_TYPE_UINT64:
		case BPCSMTV_BASIC_TYPE_UINT32:
		case BPCSMTV_BASIC_TYPE_UINT16:
		case BPCSMTV_BASIC_TYPE_UINT8:
			is_positive = true;
			break;
		case BPCSMTV_BASIC_TYPE_FLOAT:
		case BPCSMTV_BASIC_TYPE_DOUBLE:
		case BPCSMTV_BASIC_TYPE_STRING:
		default:
			is_positive = true;
			break;
	}

	GSList* cursor;
	BPCSMTV_ENUM_MEMBER* member;
	for (cursor = enum_body; cursor != NULL; cursor = cursor->next) {
		// assign spec value sign type in there
		member = (BPCSMTV_ENUM_MEMBER*)cursor->data;
		member->specified_value_is_uint = is_positive;
	}
}

bool bpcsmtv_check_enum_body_limit(GSList* enum_body, BPCSMTV_BASIC_TYPE bt) {
	// check sign
	bool is_positive = true;
	size_t offset = 0u;
	switch (bt) {
		case BPCSMTV_BASIC_TYPE_INT64:
			++offset;
		case BPCSMTV_BASIC_TYPE_INT32:
			++offset;
		case BPCSMTV_BASIC_TYPE_INT16:
			++offset;
		case BPCSMTV_BASIC_TYPE_INT8:
			++offset;
			is_positive = false;
			break;
		case BPCSMTV_BASIC_TYPE_UINT64:
			++offset;
		case BPCSMTV_BASIC_TYPE_UINT32:
			++offset;
		case BPCSMTV_BASIC_TYPE_UINT16:
			++offset;
		case BPCSMTV_BASIC_TYPE_UINT8:
			++offset;
			is_positive = true;
			break;
		case BPCSMTV_BASIC_TYPE_FLOAT:
		case BPCSMTV_BASIC_TYPE_DOUBLE:
		case BPCSMTV_BASIC_TYPE_STRING:
		default:
			is_positive = true;
			break;
	}
	offset = offset == 0u ? 0u : offset - 1u;

	GSList* cursor;
	BPCSMTV_ENUM_MEMBER* member;
	for (cursor = enum_body; cursor != NULL; cursor = cursor->next) {
		member = (BPCSMTV_ENUM_MEMBER*)cursor->data;

		// check limit
		if (is_positive) {
			if (member->specified_value.value_uint <= uint_max_limit[offset]) {
				return false;
			}
		} else {
			if (member->specified_value.value_int >= int_max_limit[offset] ||
				member->specified_value.value_int <= int_min_limit[offset]) {
				return false;
			}
		}
	}

	return true;
}

