#pragma once

#include "bpc_semantic_values.h"
#include <stdint.h>
#include <stdbool.h>
#include "bpc_cmd.h"

typedef struct _BPCGEN_UNDERLAYING_MEMBER {
	/// <summary>
	/// storage variable delivered by semantic value module
	/// </summary>
	BPCSMTV_MEMBER* semantic_value;
	/// <summary>
	/// the status of detecting underlaying type of this member, after resolving its type.
	/// when it is false, it mean that this member is must be a struct and 
	/// shoule be serialized or deserialized by calling functions rather than
	/// direct code.
	/// </summary>
	bool like_basic_type;
	/// <summary>
	/// the detected basic type of current member. only valid 
	/// when like_basic_type is true.
	/// </summary>
	BPCSMTV_BASIC_TYPE underlaying_basic_type;

	/// <summary>
	/// if the type of this member is an alias, this will be set as true.
	/// this field is more exactly than like_basic_type
	/// </summary>
	bool is_pure_basic_type;
	/// <summary>
	/// the c style type of member for convenient code gen.
	/// when the type of member is alias, it will be set as NULL, 
	/// otherwise, this field will be filled as a struct or enum name.
	/// </summary>
	char* c_style_type;
}BPCGEN_UNDERLAYING_MEMBER;

typedef struct _BPCGEN_MSG_EXTRA_PROPS {
	bool is_reliable;
}BPCGEN_MSG_EXTRA_PROPS;

void bpcgen_init_code_file(BPCCMD_PARSED_ARGS* bpc_args);
void bpcgen_write_document(BPCSMTV_DOCUMENT* document);
void bpcgen_free_code_file();

void _bpcgen_write_alias(BPCSMTV_ALIAS* token_data);
void _bpcgen_write_enum(BPCSMTV_ENUM* token_data);
void _bpcgen_write_struct(BPCSMTV_STRUCT* token_data);
void _bpcgen_write_msg(BPCSMTV_MSG* token_data);

void _bpcgen_write_preset_code(GSList* namespace_list);
void _bpcgen_write_conclusion_code();
void _bpcgen_write_tail_code(GSList* namespace_list);

void _bpcgen_get_underlaying_type(BPCGEN_UNDERLAYING_MEMBER* codegen_member);
void _bpcgen_gen_struct_msg_body(const char* token_name, GSList* smtv_member_list, BPCGEN_MSG_EXTRA_PROPS* msg_prop);
void _bpcgen_copy_template(FILE* target, const char* u8_template_code_file_path);
