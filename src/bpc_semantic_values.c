#include "bpc_semantic_values.h"

static const char* basic_type_dict[] = {
	"float",
	"double",

	"int8",
	"int16",
	"int32",
	"int64",
	"uint8",
	"uint16",
	"uint32",
	"uint64",

	"string"
};
static const size_t basic_type_dict_len = sizeof(basic_type_dict) / sizeof(char*);

static GSList* entry_registery = NULL;
static GSList* token_registery = NULL;

BPCSMTV_BASIC_TYPE bpc_parse_basic_type(const char* strl) {
	for (size_t i = 0; i < basic_type_dict_len; ++i) {
		if (g_str_equal(strl, basic_type_dict[i])) return (BPCSMTV_BASIC_TYPE)i;
	}

	// error
	return -1;
}

BPCSMTV_MEMBER* bpc_constructor_member() {
	return g_new0(BPCSMTV_MEMBER, 1);
}

void bpc_destructor_member(gpointer rawptr) {
	if (rawptr == NULL) return;
	BPCSMTV_MEMBER* ptr = (BPCSMTV_MEMBER*)rawptr;

	if (ptr->vname != NULL) g_free(ptr->vname);
	if (ptr->v_struct_type != NULL) g_free(ptr->v_struct_type);
	g_free(ptr);
}

void bpc_destructor_member_slist(GSList* list) {
	if (list == NULL) return;
	g_slist_free_full(list, bpc_destructor_member);
}

BPCSMTV_ENUM_BODY* bpc_constructor_enum_body() {
	return g_new0(BPCSMTV_ENUM_BODY, 1);
}

void bpc_destructor_enum_body(gpointer rawptr) {
	if (rawptr == NULL) return;
	BPCSMTV_ENUM_BODY* ptr = (BPCSMTV_ENUM_BODY*)rawptr;

	if (ptr->enum_name != NULL) g_free(ptr->enum_name);
}

void bpc_destructor_enum_body_slist(GSList* list) {
	if (list == NULL) return;
	g_slist_free_full(list, bpc_destructor_enum_body);
}

void bpc_destructor_string(gpointer rawptr) {
	if (rawptr == NULL) return;
	// free string from g_strdup
	g_free(rawptr);
}

void bpc_destructor_string_slist(GSList* list) {
	if (list == NULL) return;
	g_slist_free_full(list, bpc_destructor_string);
}

BPCSMTV_ALIAS* bpc_constructor_alias() {
	return g_new0(BPCSMTV_ALIAS, 1);
}
void bpc_destructor_alias(BPCSMTV_ALIAS* ptr) {
	if (ptr == NULL) return;
	g_free(ptr->user_type);

	g_free(ptr);
}
BPCSMTV_ENUM* bpc_constructor_enum() {
	return g_new0(BPCSMTV_ENUM, 1);
}
void bpc_destructor_enum(BPCSMTV_ENUM* ptr) {
	if (ptr == NULL) return;
	g_free(ptr->enum_name);
	bpc_destructor_enum_body_slist(ptr->enum_body);

	g_free(ptr);
}
BPCSMTV_STRUCT* bpc_constructor_struct() {
	return g_new0(BPCSMTV_STRUCT, 1);
}
void bpc_destructor_struct(BPCSMTV_STRUCT* ptr) {
	if (ptr == NULL) return;
	g_free(ptr->struct_name);
	bpc_destructor_member_slist(ptr->struct_member);

	g_free(ptr);
}
BPCSMTV_MSG* bpc_constructor_msg() {
	return g_new0(BPCSMTV_MSG, 1);
}
void bpc_destructor_msg(BPCSMTV_MSG* ptr) {
	if (ptr == NULL) return;
	g_free(ptr->msg_name);
	bpc_destructor_member_slist(ptr->msg_member);

	g_free(ptr);
}

BPCSMTV_DEFINE_GROUP* bpc_constructor_define_group() {
	return g_new0(BPCSMTV_DEFINE_GROUP, 1);
}
void bpc_destructor_define_group(BPCSMTV_DEFINE_GROUP* ptr) {
	if (ptr == NULL) return;
	switch (ptr->node_type) {
		case BPCSMTV_DEFINED_TOKEN_TYPE_ALIAS:
			bpc_destructor_alias(ptr->node_data.alias_data);
			break;
		case BPCSMTV_DEFINED_TOKEN_TYPE_ENUM:
			bpc_destructor_enum(ptr->node_data.enum_data);
			break;
		case BPCSMTV_DEFINED_TOKEN_TYPE_STRUCT:
			bpc_destructor_struct(ptr->node_data.struct_data);
			break;
		case BPCSMTV_DEFINED_TOKEN_TYPE_MSG:
			bpc_destructor_msg(ptr->node_data.msg_data);
			break;
	}

	g_free(ptr);
}

void bpc_destructor_define_group_slist(GSList* list) {
	if (list == NULL) return;
	g_slist_free_full(list, bpc_destructor_define_group);
}

BPCSMTV_DOCUMENT* bpc_constructor_document() {
	return g_new0(BPCSMTV_DOCUMENT, 1);
}
void bpc_destructor_document(BPCSMTV_DOCUMENT* ptr) {
	if (ptr == NULL) return;
	bpc_destructor_string_slist(ptr->namespace_data);
	bpc_destructor_define_group(ptr->define_group_data);

	g_free(ptr);
}

void bpcsmtv_member_copy_array_prop(GSList* ls, BPCSMTV_MEMBER_ARRAY_PROP* data) {
	if (ls == NULL || data == NULL) return;

	GSList* cursor;
	BPCSMTV_MEMBER* ptr;
	for (cursor = entry_registery; cursor != NULL; cursor = cursor->next) {
		ptr = (BPCSMTV_MEMBER*)cursor->data;

		ptr->array_prop.is_array = data->is_array;
		ptr->array_prop.is_static_array = data->is_static_array;
		ptr->array_prop.array_len = data->array_len;
	}
}

void bpcsmtv_member_copy_align_prop(GSList* ls, BPCSMTV_MEMBER_ALIGN_PROP* data) {
	if (ls == NULL || data == NULL) return;

	GSList* cursor;
	BPCSMTV_MEMBER* ptr;
	for (cursor = entry_registery; cursor != NULL; cursor = cursor->next) {
		ptr = (BPCSMTV_MEMBER*)cursor->data;

		ptr->align_prop.use_align = data->use_align;
		ptr->align_prop.padding_size = data->padding_size;
	}
}

void bpcsmtv_entry_registery_reset() {
	if (entry_registery != NULL) {
		bpc_destructor_string_slist(entry_registery);
		entry_registery = NULL;
	}
}
bool bpcsmtv_entry_registery_test(char* name) {
	GSList* cursor;
	for (cursor = entry_registery; cursor != NULL; cursor = cursor->next) {
		if (g_str_equal(cursor->data, name)) return true;
	}

	// no match
	// add into list and return false
	g_slist_append(entry_registery, name);
	return false;
}
void bpcsmtv_token_registery_reset() {
	if (token_registery != NULL) {
		bpc_destructor_string_slist(token_registery);
		token_registery = NULL;
	}
}
bool bpcsmtv_token_registery_test(char* name) {
	GSList* cursor;
	for (cursor = token_registery; cursor != NULL; cursor = cursor->next) {
		if (g_str_equal(cursor->data, name)) return true;
	}

	// no match
	// add into list and return false
	g_slist_append(token_registery, name);
	return false;
}

bool bpcsmtv_basic_type_is_suit_for_enum(BPCSMTV_BASIC_TYPE bt) {
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
		default:
			return false;
	}
}
