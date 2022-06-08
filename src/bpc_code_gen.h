#pragma once

#include "bpc_semantic_values.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum __bpc_codegen_token_type {
	bpc_codegen_token_type_enum,
	bpc_codegen_token_type_struct,
	bpc_codegen_token_type_msg
}_bpc_codegen_token_type;

typedef struct __bpc_codegen_token_entry {
	char* token_name;
	_bpc_codegen_token_type token_type;

	uint32_t token_arranged_index;	// only valid in msg type

	bool token_is_basic_type;	// only valid in enum type
	BPC_SEMANTIC_BASIC_TYPE token_basic_type;	// only valid in enum type
}_bpc_codegen_token_entry;

// call in order
bool bpc_codegen_init_code_file(const char* filepath);
void bpc_codegen_init_language(BPC_SEMANTIC_LANGUAGE lang);
void bpc_codegen_init_namespace(GList* namespace_chain);
// end call in order

void bpc_codegen_write_alias(const char* alias_name, BPC_SEMANTIC_BASIC_TYPE basic_t);
void bpc_codegen_write_enum(const char* enum_name, BPC_SEMANTIC_BASIC_TYPE basic_type, GSList* member_list);
void bpc_codegen_write_struct(const char* struct_name, GSList* member_list);
void bpc_codegen_write_msg(const char* msg_name, GSList* member_list);

// call in order
void bpc_codegen_write_opcode();
void bpc_codegen_free_code_file();
// end call in order

void _bpc_codegen_gen_struct_msg_body(GSList* member_list);
void _bpc_codegen_copy_template(const char* template_code_file_path);
void _bpc_codegen_free_token_entry(gpointer rawptr);
