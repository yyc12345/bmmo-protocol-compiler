#include "bpc_semantic_values.h"

const char* language_dict[] = {
	"csharp",
	"python",
	"cpp"
};
const size_t language_dict_len = sizeof(language_dict) / sizeof(char*);

const char* basic_type_dict[] = {
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
const size_t basic_type_dict_len = sizeof(basic_type_dict) / sizeof(char*);


BPC_SEMANTIC_LANGUAGE bpc_parse_language_string(const char* strl) {
	for (size_t i = 0; i < language_dict_len; ++i) {
		if (g_str_equal(strl, language_dict[i])) return (BPC_SEMANTIC_LANGUAGE)i;
	}

	// error
	return -1;
}

BPC_SEMANTIC_BASIC_TYPE bpc_parse_basic_type_string(const char* strl) {
	for (size_t i = 0; i < basic_type_dict_len; ++i) {
		if (g_str_equal(strl, basic_type_dict[i])) return (BPC_SEMANTIC_LANGUAGE)i;
	}

	// error
	return -1;
}

BPC_SEMANTIC_MEMBER* bpc_constructor_semantic_member() {
	return g_new0(BPC_SEMANTIC_MEMBER, 1);
}

void bpc_destructor_semantic_member(gpointer rawptr) {
	if (rawptr == NULL) return;

	BPC_SEMANTIC_MEMBER* ptr = (BPC_SEMANTIC_MEMBER*)rawptr;
	
	if (ptr->vname != NULL) g_free(ptr->vname);
	if (ptr->v_struct_type != NULL) g_free(ptr->v_struct_type);
	g_free(ptr);
}

void bpc_destructor_semantic_member_slist(GSList* list) {
	if (list == NULL) return;
	g_slist_free_full(list, bpc_destructor_semantic_member);
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

void bpc_lambda_semantic_member_copy_array_prop(gpointer raw_item, gpointer raw_data) {
	if (raw_data == NULL || raw_item == NULL) return;

	BPC_SEMANTIC_MEMBER* ptr = (BPC_SEMANTIC_MEMBER*)raw_item;
	BPC_SEMANTIC_MEMBER_ARRAY_PROP* data = (BPC_SEMANTIC_MEMBER_ARRAY_PROP*)raw_data;

	ptr->array_prop.is_array = data->is_array;
	ptr->array_prop.is_static_array = data->is_static_array;
	ptr->array_prop.array_len = data->array_len;
}
