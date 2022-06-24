#pragma once

#include <glib.h>
#include <stdbool.h>
#include <stdint.h>

#define BPC_COMPILER_VERSION 1

typedef enum _BPC_SEMANTIC_BASIC_TYPE {
	BPC_SEMANTIC_BASIC_TYPE_FLOAT,
	BPC_SEMANTIC_BASIC_TYPE_DOUBLE,
	BPC_SEMANTIC_BASIC_TYPE_INT8,
	BPC_SEMANTIC_BASIC_TYPE_INT16,
	BPC_SEMANTIC_BASIC_TYPE_INT32,
	BPC_SEMANTIC_BASIC_TYPE_INT64,
	BPC_SEMANTIC_BASIC_TYPE_UINT8,
	BPC_SEMANTIC_BASIC_TYPE_UINT16,
	BPC_SEMANTIC_BASIC_TYPE_UINT32,
	BPC_SEMANTIC_BASIC_TYPE_UINT64,
	BPC_SEMANTIC_BASIC_TYPE_STRING
}BPC_SEMANTIC_BASIC_TYPE;

typedef enum _BPC_SEMANTIC_LANGUAGE {
	BPC_SEMANTIC_LANGUAGE_CSHARP,
	BPC_SEMANTIC_LANGUAGE_PYTHON,
	BPC_SEMANTIC_LANGUAGE_CPP
}BPC_SEMANTIC_LANGUAGE;

typedef struct _BPC_SEMANTIC_MEMBER_ARRAY_PROP {
	bool is_array;
	bool is_static_array;
	uint32_t array_len;
}BPC_SEMANTIC_MEMBER_ARRAY_PROP;
typedef struct _BPC_SEMANTIC_MEMBER_ALIGN_PROP {
	bool use_align;
	uint32_t padding_size;
}BPC_SEMANTIC_MEMBER_ALIGN_PROP;

typedef struct _BPC_SEMANTIC_MEMBER {
	bool is_basic_type;
	BPC_SEMANTIC_BASIC_TYPE v_basic_type;
	char* v_struct_type;

	char* vname;

	BPC_SEMANTIC_MEMBER_ARRAY_PROP array_prop;
	BPC_SEMANTIC_MEMBER_ALIGN_PROP align_prop;
}BPC_SEMANTIC_MEMBER;


BPC_SEMANTIC_LANGUAGE bpc_parse_language_string(const char*);
BPC_SEMANTIC_BASIC_TYPE bpc_parse_basic_type_string(const char*);

BPC_SEMANTIC_MEMBER* bpc_constructor_semantic_member();
void bpc_destructor_semantic_member(gpointer rawptr);
void bpc_destructor_semantic_member_slist(GSList* list);

void bpc_destructor_string(gpointer rawptr);
void bpc_destructor_string_slist(GSList* list);

void bpc_lambda_semantic_member_copy_array_prop(gpointer raw_item, gpointer raw_data);
void bpc_lambda_semantic_member_copy_align_prop(gpointer raw_item, gpointer raw_data);
