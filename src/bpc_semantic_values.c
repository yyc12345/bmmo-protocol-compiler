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

void bpc_destructor_semantic_member(BPC_SEMANTIC_MEMBER* ptr) {
	if (ptr->vname != NULL) g_free(ptr->vname);
	if (ptr->v_basic_type != NULL) g_free(ptr->v_basic_type);
	g_free(ptr);
}
