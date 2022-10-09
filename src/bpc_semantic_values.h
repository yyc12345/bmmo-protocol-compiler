#pragma once

#include <glib.h>
#include <stdbool.h>
#include <stdint.h>

// SMTV stands for `semantic_value`

// ==================== Struct Defination ====================

// Invalid basic type const value
#define BPCSMTV_BASIC_TYPE_INVALID (-1);
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

typedef struct _BPCSMTV_STRUCT_MODIFIER {
	bool has_set_reliability;
	bool is_reliable;
	bool has_set_field_layout;
	bool is_narrow;
	uint32_t struct_size;
}BPCSMTV_STRUCT_MODIFIER;

typedef struct _BPCSMTV_VARIABLE_ARRAY {
	bool is_array;
	bool is_static_array;
	uint32_t static_array_len;
}BPCSMTV_VARIABLE_ARRAY;

typedef struct _BPCSMTV_VARIABLE_TYPE {
	/// <summary>
	/// whether this member is belong to basic type.
	/// this basic type do not include enum
	/// enum is struct type in there
	/// enum will only be seen as basic type in codegen.
	/// </summary>
	bool is_basic_type;

	/// <summary>
	/// the internal data of current variable type
	/// </summary>
	union {
		/// <summary>
		/// if member is basic type, this value store its basic type
		/// </summary>
		BPCSMTV_BASIC_TYPE basic_type;
		/// <summary>
		/// if member is custom type, this value store the token name of its custom type
		/// </summary>
		char* custom_type;
	}type_data;

	/// <summary>
	/// Whether field is basic type, after resolving with alias and enum.
	/// When it is false, it mean that this member is must be a struct and 
	/// shoule be serialized or deserialized by calling functions rather than
	/// direct code.
	/// </summary>
	bool full_uncover_is_basic_type;
	/// <summary>
	/// The detected basic type of current member. 
	/// Only valid when `full_uncover_is_basic_type` is true.
	/// If you want to get custom type, please use `VARIABLE_TYPE::type_data::custom_type`.
	/// </summary>
	BPCSMTV_BASIC_TYPE full_uncover_basic_type;

	/// <summary>
	/// Whether field is basic type, after resolving with alias, without resolving enum.
	/// This bool flag is do a partial resolving comparing with `full_uncover_is_basic_type`.
	/// So if this flag is true, `full_uncover_is_basic_type` must be true.
	/// </summary>
	bool semi_uncover_is_basic_type;
	/// <summary>
	/// The c style type of member for convenient code gen. 
	/// Only valid when `semi_uncover_is_basic_type` is false. 
	/// If you want to get basic type, use `full_uncover_basic_type` instead.
	/// </summary>
	char* semi_uncover_custom_type;
}BPCSMTV_VARIABLE_TYPE;

typedef struct _BPCSMTV_VARIABLE_ALIGN {
	bool use_align;
	uint32_t padding_size;
}BPCSMTV_VARIABLE_ALIGN;

typedef struct _BPCSMTV_VARIABLE {
	/// <summary>
	/// the data type of this variable
	/// </summary>
	BPCSMTV_VARIABLE_TYPE* variable_type;
	/// <summary>
	/// the array property of this variable
	/// </summary>
	BPCSMTV_VARIABLE_ARRAY* variable_array;
	/// <summary>
	/// the align property of this variable
	/// </summary>
	BPCSMTV_VARIABLE_ALIGN* variable_align;
	/// <summary>
	/// the name of this variable
	/// </summary>
	char* variable_name;
}BPCSMTV_VARIABLE;

typedef struct _BPCSMTV_ENUM_MEMBER {
	/// <summary>
	/// the name of current enum entry
	/// </summary>
	char* enum_member_name;
	/// <summary>
	/// whether current enum entry have user specificed value.
	/// </summary>
	bool have_specific_value;
	/// <summary>
	/// store user specificed value if it has.
	/// </summary>
	int64_t specific_value;
}BPCSMTV_ENUM_MEMBER;

typedef struct _BPCSMTV_ALIAS {
	/// <summary>
	/// user defined new identifier
	/// </summary>
	char* custom_type;
	/// <summary>
	/// original basic_type
	/// </summary>
	BPCSMTV_BASIC_TYPE basic_type;
}BPCSMTV_ALIAS;
typedef struct _BPCSMTV_ENUM {
	/// <summary>
	/// the identifier of this enum
	/// </summary>
	char* enum_name;
	/// <summary>
	/// the basic type used by this enum
	/// </summary>
	BPCSMTV_BASIC_TYPE enum_basic_type;
	/// <summary>
	/// item is `BPCSMTV_ENUM_BODY*`
	/// </summary>
	GSList* enum_body;
}BPCSMTV_ENUM;
typedef struct _BPCSMTV_STRUCT {
	/// <summary>
	/// modifier property
	/// </summary>
	BPCSMTV_STRUCT_MODIFIER* struct_modifier;
	/// <summary>
	/// the identifier of this struct
	/// </summary>
	char* struct_name;
	/// <summary>
	/// item is `BPCSMTV_VARIABLE*`
	/// </summary>
	GSList* struct_body;
}BPCSMTV_STRUCT;
typedef struct _BPCSMTV_MSG {
	/// <summary>
	/// modifier property
	/// </summary>
	BPCSMTV_STRUCT_MODIFIER* msg_modifier;
	/// <summary>
	/// the identifier of this msg
	/// </summary>
	char* msg_name;
	/// <summary>
	/// item is `BPCSMTV_VARIABLE*`
	/// </summary>
	GSList* msg_body;
	/// <summary>
	/// the msg index distributed by compiler
	/// </summary>
	uint32_t msg_index;
}BPCSMTV_MSG;

typedef enum _BPCSMTV_DEFINED_IDENTIFIER_TYPE {
	BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS,
	BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM,
	BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT,
	BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG
}BPCSMTV_DEFINED_IDENTIFIER_TYPE;

typedef struct _BPCSMTV_PROTOCOL_BODY {
	BPCSMTV_DEFINED_IDENTIFIER_TYPE node_type;
	union {
		BPCSMTV_ALIAS* alias_data;
		BPCSMTV_ENUM* enum_data;
		BPCSMTV_STRUCT* struct_data;
		BPCSMTV_MSG* msg_data;
	}node_data;
}BPCSMTV_PROTOCOL_BODY;

typedef struct _BPCSMTV_DOCUMENT {
	/// <summary>
	/// item is the same as the semantic value of bpc_namespace_chain and bpc_namespace, char*
	/// </summary>
	GSList* namespace_data;
	/// <summary>
	/// storage define group data.
	/// item is `BPCSMTV_PROTOCOL_BODY*`
	/// </summary>
	GSList* protocol_body;
}BPCSMTV_DOCUMENT;

// ==================== Parse Functions ====================

/// <summary>
/// parse string into BPCSMTV_BASIC_TYPE.
/// </summary>
/// <param name="strl">the string need convertion</param>
/// <returns>return a valid BPCSMTV_BASIC_TYPE enum entry, otherwise return BPCSMTV_BASIC_TYPE_INVALID.</returns>
BPCSMTV_BASIC_TYPE bpcsmtv_parse_basic_type(const char* strl);
/// <summary>
/// parse string to is_reliable
/// </summary>
/// <param name="strl"></param>
/// <returns>return true if reliability is reliable.</returns>
bool bpcsmtv_parse_reliability(const char* strl);
/// <summary>
/// parse string to is_narrow
/// </summary>
/// <param name="strl"></param>
/// <returns>return true if member_layout is narrow.</returns>
bool bpcsmtv_parse_field_layout(const char* strl);
/// <summary>
/// parse string to uint32_t
/// </summary>
/// <param name="strl"></param>
/// <returns></returns>
gint64 bpcsmtv_parse_number(const char* strl, size_t len, size_t start_margin, size_t end_margin);

// ==================== Constructor/Deconstructor Functions ====================

BPCSMTV_VARIABLE_ARRAY* bpcsmtv_constructor_variable_array();
BPCSMTV_VARIABLE_ARRAY* bpcsmtv_duplicator_variable_array(BPCSMTV_VARIABLE_ARRAY* data);
void bpcsmtv_destructor_variable_array(BPCSMTV_VARIABLE_ARRAY* data);
BPCSMTV_VARIABLE_TYPE* bpcsmtv_constructor_variable_type();
BPCSMTV_VARIABLE_TYPE* bpcsmtv_duplicator_variable_type(BPCSMTV_VARIABLE_TYPE* data);
void bpcsmtv_destructor_variable_type(BPCSMTV_VARIABLE_TYPE* data);
BPCSMTV_VARIABLE_ALIGN* bpcsmtv_constructor_variable_align();
BPCSMTV_VARIABLE_ALIGN* bpcsmtv_duplicator_variable_align(BPCSMTV_VARIABLE_ALIGN* data);
void bpcsmtv_destructor_variable_align(BPCSMTV_VARIABLE_ALIGN* data);

BPCSMTV_ALIAS* bpcsmtv_constructor_alias();
void bpcsmtv_destructor_alias(BPCSMTV_ALIAS* data);
BPCSMTV_ENUM* bpcsmtv_constructor_enum();
void bpcsmtv_destructor_enum(BPCSMTV_ENUM* data);
BPCSMTV_STRUCT* bpcsmtv_constructor_struct();
void bpcsmtv_destructor_struct(BPCSMTV_STRUCT* data);
BPCSMTV_MSG* bpcsmtv_constructor_msg();
void bpcsmtv_destructor_msg(BPCSMTV_MSG* data);

BPCSMTV_STRUCT_MODIFIER* bpcsmtv_constructor_struct_modifier();
void bpcsmtv_destructor_struct_modifier(BPCSMTV_STRUCT_MODIFIER* data);
BPCSMTV_VARIABLE* bpcsmtv_constructor_variable();
void bpcsmtv_destructor_variable(gpointer data);
BPCSMTV_ENUM_MEMBER* bpcsmtv_constructor_enum_member();
void bpcsmtv_destructor_enum_member(gpointer data);
BPCSMTV_PROTOCOL_BODY* bpcsmtv_constructor_protocol_body();
void bpcsmtv_destructor_protocol_body(gpointer data);
BPCSMTV_DOCUMENT* bpcsmtv_constructor_document();
void bpcsmtv_destructor_document(BPCSMTV_DOCUMENT* data);

void bpcsmtv_destructor_string(gpointer data);
void bpcsmtv_destructor_slist_string(GSList* data);
void bpcsmtv_destructor_slist_protocol_body(GSList* data);
void bpcsmtv_destructor_slist_enum_member(GSList* data);
void bpcsmtv_destructor_slist_variable(GSList* data);

// ==================== Registery Functions ====================
// registery_variables is used for members of each struct.
// registery_identifier is used for identifiers of protocol body.

void bpcsmtv_registery_variables_reset();
/// <summary>
/// 
/// </summary>
/// <param name="name"></param>
/// <returns>return true if variable name is existed.</returns>
bool bpcsmtv_registery_variables_test(const char* name);
void bpcsmtv_registery_variables_add(const char* name);

void bpcsmtv_registery_identifier_reset();
/// <summary>
/// 
/// </summary>
/// <param name="name"></param>
/// <returns>return true if identifier is existed.</returns>
bool bpcsmtv_registery_identifier_test(const char* name);
BPCSMTV_PROTOCOL_BODY* bpcsmtv_registery_identifier_get(const char* name);
uint32_t bpcsmtv_registery_identifier_distribute_index();
void bpcsmtv_registery_identifier_add(BPCSMTV_PROTOCOL_BODY* data);

// ==================== Utils Functions ====================

/// <summary>
/// check whether current number is suit for offset.
/// </summary>
/// <param name="num"></param>
/// <returns>if number is lower than max(uint32_t) and not equal with 0, return true.</returns>
bool bpcsmtv_is_offset_number(gint64 num);
bool bpcsmtv_is_basic_type_suit_for_enum(BPCSMTV_BASIC_TYPE bt);
bool bpcsmtv_is_modifier_suit_struct(BPCSMTV_STRUCT_MODIFIER* modifier);
/// <summary>
/// called when preparing struct/msg body to fulfill field layout data.
/// </summary>
/// <param name="variables"></param>
/// <param name="modifier"></param>
void bpcsmtv_setup_field_layout(GSList* variables, BPCSMTV_STRUCT_MODIFIER* modifier);
void bpcsmtv_setup_reliability(BPCSMTV_STRUCT_MODIFIER* modifier);
void bpcsmtv_analyse_underlaying_type(BPCSMTV_VARIABLE_TYPE* variables);

/*

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

*/

/*
We call `entry` as:

- The field of struct and msg.
- The enum item in enum.
- The user defined alias name in alias.

We call `token` as:

- The name of struct, msg and enum self.

*/

/*
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

*/