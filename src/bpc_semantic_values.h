#pragma once

#include <glib.h>
#include <stdbool.h>
#include <stdint.h>

// SMTV stands for `semantic_value`

typedef enum _BPCSMTV_BASIC_TYPE {
	BPCSMTV_BASIC_TYPE_FLOAT,
	BPCSMTV_BASIC_TYPE_DOUBLE,
	BPCSMTV_BASIC_TYPE_INT8,
	BPCSMTV_BASIC_TYPE_INT16,
	BPCSMTV_BASIC_TYPE_INT32,
	BPCSMTV_BASIC_TYPE_INT64,
	BPCSMTV_BASIC_TYPE_UINT8,
	BPCSMTV_BASIC_TYPE_UINT16,
	BPCSMTV_BASIC_TYPE_UINT32,
	BPCSMTV_BASIC_TYPE_UINT64,
	BPCSMTV_BASIC_TYPE_STRING
}BPCSMTV_BASIC_TYPE;

typedef struct _BPCSMTV_ENUM_BODY {
	/// <summary>
	/// the name of current enum entry
	/// </summary>
	char* enum_name;

	/// <summary>
	/// whether current enum entry have user specificed value.
	/// </summary>
	bool have_specific_value;
	/// <summary>
	/// store user specificed value if it has.
	/// </summary>
	int64_t specific_value;
}BPCSMTV_ENUM_BODY;

typedef struct _BPC_SEMANTIC_MEMBER_ARRAY_PROP {
	bool is_array;
	bool is_static_array;
	uint32_t array_len;
}BPCSMTV_MEMBER_ARRAY_PROP;
typedef struct _BPC_SEMANTIC_MEMBER_ALIGN_PROP {
	bool use_align;
	uint32_t padding_size;
}BPCSMTV_MEMBER_ALIGN_PROP;

typedef struct _BPCSMTV_MEMBER {
	/// <summary>
	/// whether this member is belong to basic type.
	/// this basic type do not include enum
	/// enum is struct type in there
	/// enum will only be seen as basic type in codegen.
	/// </summary>
	bool is_basic_type;
	/// <summary>
	/// if member is basic type, this value store its basic type
	/// </summary>
	BPCSMTV_BASIC_TYPE v_basic_type;
	/// <summary>
	/// if member is struct type, this value store the token name of its struct type
	/// </summary>
	char* v_struct_type;

	/// <summary>
	/// the token name of member
	/// </summary>
	char* vname;

	/// <summary>
	/// the array props of member
	/// </summary>
	BPCSMTV_MEMBER_ARRAY_PROP array_prop;
	/// <summary>
	/// the align props of member
	/// </summary>
	BPCSMTV_MEMBER_ALIGN_PROP align_prop;
}BPCSMTV_MEMBER;

typedef struct _BPCSMTV_ALIAS {
	char* user_type;
	BPCSMTV_BASIC_TYPE basic_type;
}BPCSMTV_ALIAS;
typedef struct _BPCSMTV_ENUM {
	char* enum_name;
	BPCSMTV_BASIC_TYPE enum_basic_type;
	/// <summary>
	/// item is `BPCSMTV_ENUM_BODY*`
	/// </summary>
	GSList* enum_body;
}BPCSMTV_ENUM;
typedef struct _BPCSMTV_STRUCT {
	char* struct_name;
	/// <summary>
	/// item is `BPCSMTV_MEMBER*`
	/// </summary>
	GSList* struct_member;
}BPCSMTV_STRUCT;
typedef struct _BPCSMTV_MSG {
	char* msg_name;
	bool is_reliable;
	/// <summary>
	/// item is `BPCSMTV_MEMBER*`
	/// </summary>
	GSList* msg_member;
}BPCSMTV_MSG;

typedef enum _BPCSMTV_DEFINED_TOKEN_TYPE {
	BPCSMTV_DEFINED_TOKEN_TYPE_ALIAS,
	BPCSMTV_DEFINED_TOKEN_TYPE_ENUM,
	BPCSMTV_DEFINED_TOKEN_TYPE_STRUCT,
	BPCSMTV_DEFINED_TOKEN_TYPE_MSG
}BPCSMTV_DEFINED_TOKEN_TYPE;

typedef struct _BPCSMTV_DEFINE_GROUP {
	BPCSMTV_DEFINED_TOKEN_TYPE node_type;
	union {
		BPCSMTV_ALIAS* alias_data;
		BPCSMTV_ENUM* enum_data;
		BPCSMTV_STRUCT* struct_data;
		BPCSMTV_MSG* msg_data;
	}node_data;
}BPCSMTV_DEFINE_GROUP;

typedef struct _BPCSMTV_DOCUMENT {
	/// <summary>
	/// item is the same as the semantic value of bpc_namespace_chain and bpc_namespace, char*
	/// </summary>
	GSList* namespace_data;
	/// <summary>
	/// storage define group data.
	/// item is `BPCSMTV_DEFINE_GROUP*`
	/// </summary>
	GSList* define_group_data;
}BPCSMTV_DOCUMENT;

/// <summary>
/// the struct recording the properties of each token 
/// including alias, enum, struct and msg
/// </summary>
typedef struct _BPCSMTV_TOKEN_REGISTERY_ITEM {
	/// <summary>
	/// the name of this token
	/// </summary>
	char* token_name;
	/// <summary>
	/// the type of this token: alias, enum, struct and msg
	/// </summary>
	BPCSMTV_DEFINED_TOKEN_TYPE token_type;

	/// <summary>
	/// some extra props of token
	/// </summary>
	union {
		/// <summary>
		/// the distributed ordered index of `msg` type. 
		/// only valid in msg type
		/// </summary>
		uint32_t token_arranged_index;
		/// <summary>
		/// underlaying basic type. 
		/// only valid in enum and alias type. 
		/// </summary>
		BPCSMTV_BASIC_TYPE token_basic_type;
	}token_extra_props;

}BPCSMTV_TOKEN_REGISTERY_ITEM;


BPCSMTV_BASIC_TYPE bpc_parse_basic_type(const char*);

BPCSMTV_MEMBER* bpc_constructor_member();
void bpc_destructor_member(gpointer rawptr);
void bpc_destructor_member_slist(GSList* list);

BPCSMTV_ENUM_BODY* bpc_constructor_enum_body();
void bpc_destructor_enum_body(gpointer rawptr);
void bpc_destructor_enum_body_slist(GSList* list);

void bpc_destructor_string(gpointer rawptr);
void bpc_destructor_string_slist(GSList* list);

BPCSMTV_ALIAS* bpc_constructor_alias();
void bpc_destructor_alias(BPCSMTV_ALIAS* ptr);
BPCSMTV_ENUM* bpc_constructor_enum();
void bpc_destructor_enum(BPCSMTV_ENUM* ptr);
BPCSMTV_STRUCT* bpc_constructor_struct();
void bpc_destructor_struct(BPCSMTV_STRUCT* ptr);
BPCSMTV_MSG* bpc_constructor_msg();
void bpc_destructor_msg(BPCSMTV_MSG* ptr);

BPCSMTV_DEFINE_GROUP* bpc_constructor_define_group();
void bpc_destructor_define_group(BPCSMTV_DEFINE_GROUP* ptr);
void bpc_destructor_define_group_slist(GSList* list);

BPCSMTV_DOCUMENT* bpc_constructor_document();
void bpc_destructor_document(BPCSMTV_DOCUMENT* ptr);

GSList* bpcsmtv_member_duplicate(GSList* refls, const char* new_name);
void bpcsmtv_member_copy_array_prop(GSList* ls, BPCSMTV_MEMBER_ARRAY_PROP* data);
void bpcsmtv_member_copy_align_prop(GSList* ls, BPCSMTV_MEMBER_ALIGN_PROP* data);

/*
We call `entry` as:

- The field of struct and msg.
- The enum item in enum.
- The user defined alias name in alias.

We call `token` as:

- The name of struct, msg and enum self.

*/

/// <summary>
/// reset entry registery. 
/// only should be reset when a block(enum, msg, struct) has been parsed.
/// </summary>
void bpcsmtv_entry_registery_reset();
/// <summary>
/// test specific `entry`
/// </summary>
/// <param name="name"></param>
/// <returns>return true if exist, otherwise return false</returns>
bool bpcsmtv_entry_registery_test(const char* name);
void bpcsmtv_entry_registery_add(const char* name);

void bpcsmtv_token_registery_reset();
/// <summary>
/// test specific `token`
/// </summary>
/// <param name="name"></param>
/// <returns>return true if exist, otherwise return false</returns>
bool bpcsmtv_token_registery_test(const char* name);
BPCSMTV_TOKEN_REGISTERY_ITEM* bpcsmtv_token_registery_get(const char* name);
void bpcsmtv_token_registery_add_alias(BPCSMTV_ALIAS* data);
void bpcsmtv_token_registery_add_enum(BPCSMTV_ENUM* data);
void bpcsmtv_token_registery_add_struct(BPCSMTV_STRUCT* data);
void bpcsmtv_token_registery_add_msg(BPCSMTV_MSG* data);
GSList* bpcsmtv_token_registery_get_slist();

bool bpcsmtv_basic_type_is_suit_for_enum(BPCSMTV_BASIC_TYPE bt);

