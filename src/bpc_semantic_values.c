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

/// <summary>
/// item is `string` of each entry's name
/// </summary>
static GSList* entry_registery = NULL;
/// <summary>
/// item is `BPCSMTV_TOKEN_REGISTERY_ITEM`
/// </summary>
static GSList* token_registery = NULL;
static uint32_t msg_index_distributor = 0;

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
	bpc_destructor_define_group_slist(ptr->define_group_data);

	g_free(ptr);
}

GSList* bpcsmtv_member_duplicate(GSList* refls, const char* new_name) {
	BPCSMTV_MEMBER* references_item = (BPCSMTV_MEMBER*)refls->data;

	// construct new item and apply some properties from
	// references item
	BPCSMTV_MEMBER* data = bpc_constructor_member();
	data->vname = (char*)new_name;
	data->is_basic_type = references_item->is_basic_type;
	if (data->is_basic_type) {
		data->v_basic_type = references_item->v_basic_type;
	} else {
		// use strdup to ensure each item have unique string in memory
		// for safely free memory for each item.
		data->v_struct_type = g_strdup(references_item->v_struct_type);
	}

	return g_slist_append(refls, data);
}

void bpcsmtv_member_copy_array_prop(GSList* ls, BPCSMTV_MEMBER_ARRAY_PROP* data) {
	if (ls == NULL || data == NULL) return;

	GSList* cursor;
	BPCSMTV_MEMBER* ptr;
	for (cursor = ls; cursor != NULL; cursor = cursor->next) {
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
	for (cursor = ls; cursor != NULL; cursor = cursor->next) {
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

bool bpcsmtv_entry_registery_test(const char* name) {
	GSList* cursor;
	for (cursor = entry_registery; cursor != NULL; cursor = cursor->next) {
		if (g_str_equal(cursor->data, name)) return true;
	}

	// no match
	return false;
}

void bpcsmtv_entry_registery_add(const char* name) {
	g_slist_append(entry_registery, (gpointer)name);
}


void bpcsmtv_token_registery_reset() {
	if (token_registery != NULL) {
		GSList* cursor;
		BPCSMTV_TOKEN_REGISTERY_ITEM* data;
		for (cursor = token_registery; cursor != NULL; cursor = cursor->next) {
			data = (BPCSMTV_TOKEN_REGISTERY_ITEM*)cursor->data;
			g_free(data->token_name);
			g_free(data);
		}
		token_registery = NULL;
	}
	msg_index_distributor = 0;
}

bool bpcsmtv_token_registery_test(const char* name) {
	return bpcsmtv_token_registery_get(name) != NULL;
}

BPCSMTV_TOKEN_REGISTERY_ITEM* bpcsmtv_token_registery_get(const char* name) {
	GSList* cursor;
	for (cursor = token_registery; cursor != NULL; cursor = cursor->next) {
		BPCSMTV_TOKEN_REGISTERY_ITEM* data = (BPCSMTV_TOKEN_REGISTERY_ITEM*)cursor->data;
		if (g_str_equal(data->token_name, name)) return data;
	}

	// no match
	return NULL;
}

void bpcsmtv_token_registery_add_alias(BPCSMTV_ALIAS* data) {
	BPCSMTV_TOKEN_REGISTERY_ITEM* entry = g_new0(BPCSMTV_TOKEN_REGISTERY_ITEM, 1);
	entry->token_name = g_strdup(data->user_type);
	entry->token_type = BPCSMTV_DEFINED_TOKEN_TYPE_ALIAS;
	entry->token_extra_props.token_basic_type = data->basic_type;
	token_registery = g_slist_append(token_registery, entry);
}

void bpcsmtv_token_registery_add_enum(BPCSMTV_ENUM* data) {
	BPCSMTV_TOKEN_REGISTERY_ITEM* entry = g_new0(BPCSMTV_TOKEN_REGISTERY_ITEM, 1);
	entry->token_name = g_strdup(data->enum_name);
	entry->token_type = BPCSMTV_DEFINED_TOKEN_TYPE_ENUM;
	entry->token_extra_props.token_basic_type = data->enum_basic_type;
	token_registery = g_slist_append(token_registery, entry);
}

void bpcsmtv_token_registery_add_struct(BPCSMTV_STRUCT* data) {
	BPCSMTV_TOKEN_REGISTERY_ITEM* entry = g_new0(BPCSMTV_TOKEN_REGISTERY_ITEM, 1);
	entry->token_name = g_strdup(data->struct_name);
	entry->token_type = BPCSMTV_DEFINED_TOKEN_TYPE_STRUCT;
	token_registery = g_slist_append(token_registery, entry);
}

void bpcsmtv_token_registery_add_msg(BPCSMTV_MSG* data) {
	BPCSMTV_TOKEN_REGISTERY_ITEM* entry = g_new0(BPCSMTV_TOKEN_REGISTERY_ITEM, 1);
	entry->token_name = g_strdup(data->msg_name);
	entry->token_type = BPCSMTV_DEFINED_TOKEN_TYPE_MSG;
	entry->token_extra_props.token_arranged_index = msg_index_distributor++;	// generate msg index
	token_registery = g_slist_append(token_registery, entry);
}

GSList* bpcsmtv_token_registery_get_slist() {
	return token_registery;
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
