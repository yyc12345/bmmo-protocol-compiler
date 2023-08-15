#include "bpc_semantic_values.h"
#include "bpc_error.h"
#include <inttypes.h>

/// <summary>
/// item is `char*` of each variables name
/// </summary>
static GHashTable* hashtable_variables = NULL;
/// <summary>
/// item is `BPCSMTV_PROTOCOL_BODY*` of each identifier
/// </summary>
static GHashTable* hashtable_identifier = NULL;
static uint32_t msg_index_distributor = 0;
static GHashTable* hashtable_enum_value = NULL;

static const char* basic_type_showcase[] = {
	"float", "double",
	"int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64",
	"string"
};
static const size_t basic_type_len = sizeof(basic_type_showcase) / sizeof(char*);
static const uint32_t basic_type_sizeof[] = {
	4, 8,
	1, 2, 4, 8, 1, 2, 4, 8,
	1
};
static const uint32_t basic_type_alignof[] = {
	// we use x86 align in any platform, due to this compiler purpose.
	// so some 8bytes data types are aligned at 4bytes boundry.
	4, 4,
	1, 2, 4, 4, 1, 2, 4, 4,
	1
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
	bpcerr_panic(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Fail to parse a basic type token which definately is a valid basic type token.");
}
bool bpcsmtv_parse_reliability(const char* strl) {
	return g_str_equal(strl, "reliable");
}
bool bpcsmtv_parse_field_layout(const char* strl) {
	return g_str_equal(strl, "narrow");
}
bool bpcsmtv_parse_number(const char* strl, size_t len, size_t start_margin, size_t end_margin, BPCSMTV_COMPOUND_NUMBER* result) {
	// setup result first
	result->num_int = INT64_C(0);
	result->num_uint = UINT64_C(0);
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
		result->success_uint = g_ascii_string_to_unsigned(start_cursor, 10u, UINT64_C(0), UINT64_MAX, &(result->num_uint), NULL);
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
		default:
			g_assert_not_reached();
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
		default:
			g_assert_not_reached();
	}

	// add for hashtable
	g_hash_table_replace(hashtable_identifier, g_strdup(identifier_name), data);
}

// ==================== Utils Functions ====================

bool bpcsmtv_get_offset_number(BPCSMTV_COMPOUND_NUMBER* num, uint32_t* outnum) {
	if (outnum != NULL) *outnum = UINT32_C(0);

	if (!num->success_uint) {
		bpcerr_error(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Can not get a unsigned integer from parsed compound integer.");
		return false;
	}
	if (num->num_uint > UINT32_MAX || num->num_uint == UINT64_C(0)) {
		bpcerr_error(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Integer %" PRIu64 " is not in the legal range of offset.", num->num_uint);
		return false;
	}

	if (outnum != NULL) *outnum = (uint32_t)(num->num_uint);
	return true;
}

bool bpcsmtv_is_basic_type_suit_for_enum(BPCSMTV_BASIC_TYPE bt) {
	switch (bt) {
		case BPCSMTV_BASIC_TYPE_INT8:
		case BPCSMTV_BASIC_TYPE_INT16:
		case BPCSMTV_BASIC_TYPE_INT32:
		case BPCSMTV_BASIC_TYPE_INT64:
		case BPCSMTV_BASIC_TYPE_UINT8:
		case BPCSMTV_BASIC_TYPE_UINT16:
		case BPCSMTV_BASIC_TYPE_UINT32:
		case BPCSMTV_BASIC_TYPE_UINT64:
			return true;
		case BPCSMTV_BASIC_TYPE_FLOAT:
		case BPCSMTV_BASIC_TYPE_DOUBLE:
		case BPCSMTV_BASIC_TYPE_STRING:
			return false;
		default:
			g_assert_not_reached();
	}
}

bool bpcsmtv_is_modifier_suit_struct(BPCSMTV_STRUCT_MODIFIER* modifier) {
	return (!modifier->has_set_reliability);
}


uint32_t bpcsmtv_get_bt_size(BPCSMTV_BASIC_TYPE bt) {
	size_t v = (size_t)bt;
	if (v >= basic_type_len) return UINT32_C(0);
	return basic_type_sizeof[bt];
}

// A helper function to get variable size and alignment requirement
static void bpcsmtv_get_variable_type_size_and_align_req(BPCSMTV_VARIABLE* variable, uint32_t* tsize, uint32_t* talign_req, const char* hint_name) {
	if (variable->variable_type->full_uncover_is_basic_type) {
		// direct basic type, enum or alias
		// check len
		if (G_UNLIKELY(variable->variable_type->full_uncover_basic_type == BPCSMTV_BASIC_TYPE_STRING)) {
			bpcerr_panic(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Get string as natural elements in accident when getting basic type alignof.");
		}
		size_t index = (size_t)variable->variable_type->full_uncover_basic_type;
		if (G_UNLIKELY(index >= basic_type_len)) {
			bpcerr_panic(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Element offset overflow when getting basic type alignof.");
		}

		if (tsize != NULL) *tsize = basic_type_sizeof[index];
		if (talign_req != NULL) *talign_req = basic_type_alignof[index];
	} else {
		// struct
		// look up its natural struct
		BPCSMTV_PROTOCOL_BODY* entry = bpcsmtv_registery_identifier_get(variable->variable_type->type_data.custom_type);
		if (G_UNLIKELY(entry == NULL)) {
			bpcerr_panic(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Fail to get already defined data struct \"%s\" when analysing \"%s\".",
				variable->variable_type->type_data.custom_type, hint_name);
		}

		if (tsize != NULL) *tsize = entry->node_data.struct_data->struct_modifier->struct_size;
		if (talign_req != NULL) *talign_req = entry->node_data.struct_data->struct_modifier->struct_unit_size;
	}
}
void bpcsmtv_setup_field_layout(GSList* variables, BPCSMTV_STRUCT_MODIFIER* modifier, const char* hint_name) {
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
			// enum or alias
			if (variable->variable_type->full_uncover_basic_type == BPCSMTV_BASIC_TYPE_STRING) {
				// string is not allowed
				can_be_natural = false;
				break;
			}
		} else {
			// struct
			// look up whether it is natural struct
			BPCSMTV_PROTOCOL_BODY* entry = bpcsmtv_registery_identifier_get(variable->variable_type->type_data.custom_type);
			if (G_UNLIKELY(entry == NULL)) {
				bpcerr_panic(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Fail to get already defined data struct \"%s\".",
					variable->variable_type->type_data.custom_type);
			}

			if (entry->node_data.struct_data->struct_modifier->is_narrow) {
				// narrow struct is not allowed
				can_be_natural = false;
				break;
			}
		}
	}

	// change modifier
	if (modifier->has_set_field_layout) {
		if (!can_be_natural && !modifier->is_narrow) {
			bpcerr_warning(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, 
				"Speficied field layout modifier \"natural\" is not suit for \"%s\". we change it to \"narrow\" forcely.", hint_name);

			modifier->is_narrow = true;
		}
	} else {
		bpcerr_info(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "The field layout modifier of \"%s\" is not found, we assume it as \"%s\".",
			hint_name, (can_be_natural ? "natural" : "narrow"));

		modifier->has_set_field_layout = true;
		modifier->is_narrow = !can_be_natural;
	}

	// set align prop
	if (modifier->is_narrow) {
		// narrow mode, set to default mode
		modifier->struct_size = UINT32_C(1);
		modifier->struct_unit_size = UINT32_C(1);
	} else {
		// iterate full list to gain unit size
		modifier->struct_unit_size = UINT32_C(1);
		for (cursor = variables; cursor != NULL; cursor = cursor->next) {
			variable = (BPCSMTV_VARIABLE*)cursor->data;

			// get align size
			uint32_t align_size;
			bpcsmtv_get_variable_type_size_and_align_req(variable, NULL, &align_size, hint_name);

			// compare and get larger one.
			modifier->struct_unit_size = modifier->struct_unit_size >= align_size ?
				modifier->struct_unit_size : align_size;
		}
		
		// set align for each item
		modifier->struct_size = UINT32_C(0);
		uint32_t padding = UINT32_C(0), real_size = UINT32_C(0);
		BPCSMTV_VARIABLE* nextvar;
		for (cursor = variables; cursor != NULL; cursor = cursor->next) {
			variable = (BPCSMTV_VARIABLE*)cursor->data;
			nextvar = (cursor->next == NULL) ? NULL : ((BPCSMTV_VARIABLE*)cursor->next->data);
			
			// get this variable size
			bpcsmtv_get_variable_type_size_and_align_req(variable, &real_size, NULL, hint_name);

			// and get align boundary
			uint32_t align_boundary;
			if (nextvar == NULL) {
				// last memeber should align with struct align
				align_boundary = modifier->struct_unit_size;
			} else {
				// non-last memeber should pad blank for next variable required boundary
				bpcsmtv_get_variable_type_size_and_align_req(nextvar, NULL, &align_boundary, hint_name);
			}

			// calc real size
			// considering array
			if (variable->variable_array->is_array) {
				real_size *= variable->variable_array->static_array_len;
			}
			
			// calc padding
			// Ref: https://en.wikipedia.org/wiki/Data_structure_alignment
			padding = (align_boundary - ((modifier->struct_size + real_size) % align_boundary)) % align_boundary;
			
			// set align if necessary
			// raise warning if there is a existed align
			if (variable->variable_align->use_align) {
				bpcerr_warning(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Specified align %" PRIu32 " for variable \"%s\" in \"%s\" has been replaced due to natural struct layout", 
					variable->variable_align->padding_size, variable->variable_name, hint_name);
			}
			if (padding != UINT32_C(0)) {
				variable->variable_align->use_align = true;
				variable->variable_align->padding_size = padding;
			} else {
				variable->variable_align->use_align = false;
			}
			
			// feedback to full size
			modifier->struct_size += real_size + padding;
		}
		
		// correct full size if necessary
		if (modifier->struct_size == UINT32_C(0)) {
			modifier->struct_size = UINT32_C(1);
		}
		
	}

}

void bpcsmtv_setup_reliability(BPCSMTV_STRUCT_MODIFIER* modifier, const char* hint_name) {
	if (!modifier->has_set_reliability) {
		bpcerr_info(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "The realibility modifier of \"%s\" is not found, we assume it as \"reliable\".", hint_name);

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
		if (G_UNLIKELY(entry == NULL)) {
			bpcerr_panic(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Fail to get previous defined data type when evaluating underlaying data type.");
		}

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
				variables->full_uncover_basic_type = -1;
				variables->semi_uncover_is_basic_type = false;
				variables->semi_uncover_custom_type = g_strdup(variables->type_data.custom_type);
				break;
			case BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG:
			default:
				bpcerr_panic(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Previous defined data type is invalid when evaluating underlaying data type.");
				break;	// skip
		}
	}
}
//
void bpcsmtv_assign_enum_member_value(BPCSMTV_ENUM_MEMBER* member, BPCSMTV_COMPOUND_NUMBER* number) {
	if (number == NULL) {
		member->has_specified_value = false;
	} else {
		member->has_specified_value = true;
		// copy data when data is valid
		memcpy(&(member->specified_value), number, sizeof(BPCSMTV_COMPOUND_NUMBER));
	}
}

bool bpcsmtv_arrange_enum_body_value(GSList* enum_body, BPCSMTV_BASIC_TYPE bt, const char* hint_name) {
	// prepare value duplication detector hashtable first
	// init hashtable when necessary
	if (hashtable_enum_value == NULL) {
		// we use it as Set, so we only need to free key, otherwise, string will be freed twice.
		hashtable_enum_value = g_hash_table_new_full(g_int64_hash, g_int64_equal, g_free, NULL);
	} else {
		// clear hashtable
		g_hash_table_remove_all(hashtable_enum_value);
	}
	
	// check sign
	bool is_unsigned = true;
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
			is_unsigned = false;
			break;
		case BPCSMTV_BASIC_TYPE_UINT64:
			++offset;
		case BPCSMTV_BASIC_TYPE_UINT32:
			++offset;
		case BPCSMTV_BASIC_TYPE_UINT16:
			++offset;
		case BPCSMTV_BASIC_TYPE_UINT8:
			++offset;
			is_unsigned = true;
			break;
		case BPCSMTV_BASIC_TYPE_FLOAT:
		case BPCSMTV_BASIC_TYPE_DOUBLE:
		case BPCSMTV_BASIC_TYPE_STRING:
		default:
			is_unsigned = true;
			bpcerr_panic(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Get invalid enum basic type when analysing enum members.");
			break;
	}
	offset = offset == 0u ? 0u : offset - 1u;

	GSList* cursor;
	BPCSMTV_ENUM_MEMBER *member = NULL, *parent = NULL;
	uint64_t pending_uint = UINT64_C(0);
	int64_t pending_int = INT64_C(0);
	for (cursor = enum_body; cursor != NULL; cursor = cursor->next) {
		parent = member;
		member = (BPCSMTV_ENUM_MEMBER*)cursor->data;

		// distribute number
		if (member->has_specified_value) {
			// analyse speficied value
			if (is_unsigned) {
				// fail to parse uint
				if (!member->specified_value.success_uint) {
					bpcerr_error(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Enum \"%s\", member \"%s\" do not have parsed unsigned integer.",
						hint_name, member->enum_member_name);
					return false;
				}
				// value overflow
				if (member->specified_value.num_uint > uint_max_limit[offset]) {
					bpcerr_error(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Enum \"%s\", member \"%s\" specify too large integer %" PRIu64 ". The legal max integer is %" PRIu64 ".",
						hint_name, member->enum_member_name, member->specified_value.num_uint, uint_max_limit[offset]);
					return false;
				}

				// assign
				pending_uint = member->specified_value.num_uint;
			} else {
				// fail to parse int
				if (!member->specified_value.success_int) {
					bpcerr_error(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Enum \"%s\", member \"%s\" do not have parsed signed integer.",
						hint_name, member->enum_member_name);
					return false;
				}
				// value overflow
				if (member->specified_value.num_int > int_max_limit[offset] ||
					member->specified_value.num_int < int_min_limit[offset]) {
					bpcerr_error(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Enum \"%s\", member \"%s\" specify too large integer %" PRIi64 ". The legal min integer is %" PRIi64 ", and max integer is %" PRIi64 ".",
						hint_name, member->enum_member_name, member->specified_value.num_int, int_min_limit[offset], int_max_limit[offset]);
					return false;
				}

				// assign
				pending_int = member->specified_value.num_int;
			}
		} else {
			// try distribute one
			if (is_unsigned) {
				// set to zero when it is the first member
				if (parent == NULL) {
					pending_uint = UINT64_C(0);
				} else {
					// if overflow, shrunk to zero
					if (pending_uint >= uint_max_limit[offset]) {
						pending_uint = UINT64_C(0);
					} else {
						// otherwise, inc
						++pending_uint;
					}
				}
				
			} else {
				// set to zero when it is the first member
				if (parent == NULL) {
					pending_int = INT64_C(0);
				} else {
					// if overflow, shrunk to min value
					if (pending_int >= int_max_limit[offset]) {
						pending_int = int_min_limit[offset];
					} else {
						// otherwise, inc
						++pending_int;
					}
				}

			}
		}
		
		// check duplication
		gpointer prob = NULL;
		if (is_unsigned) prob = g_memdup2(&pending_uint, sizeof(uint64_t));
		else prob = g_memdup2(&pending_int, sizeof(int64_t));
		if (!g_hash_table_add(hashtable_enum_value, prob)) {
			// add is a macro to replace function. so in any cases, 
			// the data has been push into hashtable and we do not process it anymore,
			// including free it. the old value has been freed by hashtable self-impl and 
			// the new value has been managed by hashtable.
			if (is_unsigned) {
				bpcerr_error(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Dupicated value %" PRIu64 " of enum \"%s\", member \"%s\"",
					pending_uint, hint_name, member->enum_member_name);
			} else {
				bpcerr_error(BPCERR_ERROR_SOURCE_SEMANTIC_VALUE, "Dupicated value %" PRIi64 " of enum \"%s\", member \"%s\"",
					pending_int, hint_name, member->enum_member_name);
			}
			return false;
		}
		
		// mark sign and assign data
		member->distributed_value_is_uint = is_unsigned;
		if (is_unsigned) member->distributed_value.value_uint = pending_uint;
		else member->distributed_value.value_int = pending_int;
	}

	return true;
}
