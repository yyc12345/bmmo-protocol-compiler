#pragma once

#include <glib.h>
#include <stdbool.h>
#include <stdint.h>

// SMTV stands for `semantic_value`

// ==================== Struct Defination ====================

typedef struct _BPCSMTV_COMPOUND_NUMBER {
	bool success_uint;
	guint64 num_uint;
	bool success_int;
	gint64 num_int;
}BPCSMTV_COMPOUND_NUMBER;

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
	/// indicate the sign of `distributed_value`, this bool will determine 
	/// the union member which will be read in `distributed_value`.
	/// true when distributed value is unsigned int.
	/// </summary>
	bool distributed_value_is_uint;
	/// <summary>
	/// stored confirmed and arranged enum member value
	/// </summary>
	union {
		uint64_t value_uint;
		int64_t value_int;
	}distributed_value;

	/// <summary>
	/// to indicate whether user has specified a value for this member
	/// </summary>
	bool has_specified_value;
	/// <summary>
	/// store user specificed value if it has.
	/// </summary>
	BPCSMTV_COMPOUND_NUMBER specified_value;
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
/// parse string to int64_t
/// </summary>
/// <param name="strl">the number string</param>
/// <param name="len">the length of number string, excluding end "\0"</param>
/// <param name="start_margin"></param>
/// <param name="end_margin"></param>
/// <param name="result">pointer to final result</param>
/// <returns></returns>
bool bpcsmtv_parse_number(const char* strl, size_t len, size_t start_margin, size_t end_margin, BPCSMTV_COMPOUND_NUMBER* result);

// ==================== Constructor/Deconstructor Functions ====================

/*
- constructor: create a new instance.
- destructor: dispose a existed instance.
- duplicator: return a new allocated instance according to a existed instance.
*/

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
/// parse compound number into acceptable offset number
/// </summary>
/// <param name="num"></param>
/// <param name="outnum">the variable to receive parsed offset</param>
/// <returns>if number is lower than max(uint32_t) and not equal with 0, return true.</returns>
bool bpcsmtv_get_offset_number(BPCSMTV_COMPOUND_NUMBER* num, uint32_t* outnum);
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
/// <summary>
/// assign compound number to enum member
/// </summary>
/// <param name="member"></param>
/// <param name="number">assigned compound number. if it is NULL, assign it as no-specified value.</param>
void bpcsmtv_assign_enum_member_value(BPCSMTV_ENUM_MEMBER* member, BPCSMTV_COMPOUND_NUMBER* number);
/// <summary>
/// arrange enum body member value and check legality at the same time
/// </summary>
/// <param name="enum_body"></param>
/// <param name="bt"></param>
/// <returns></returns>
bool bpcsmtv_arrange_enum_body_value(GSList* enum_body, BPCSMTV_BASIC_TYPE bt);
