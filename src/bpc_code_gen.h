#pragma once

#include "bpc_semantic_values.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum _BPC_CODEGEN_TOKEN_TYPE {
	BPC_CODEGEN_TOKEN_TYPE_ENUM,
	BPC_CODEGEN_TOKEN_TYPE_STRUCT,
	BPC_CODEGEN_TOKEN_TYPE_MSG,
	BPC_CODEGEN_TOKEN_TYPE_ALIAS
}BPC_CODEGEN_TOKEN_TYPE;

typedef struct _BPC_CODEGEN_TOKEN_ENTRY {
	char* token_name;
	BPC_CODEGEN_TOKEN_TYPE token_type;

	uint32_t token_arranged_index;	// only valid in msg type
	BPC_SEMANTIC_BASIC_TYPE token_basic_type;	// only valid in enum and alias type
}BPC_CODEGEN_TOKEN_ENTRY;




// call in order
bool bpc_codegen_init_code_file(const char* filepath);
void bpc_codegen_init_language(BPC_SEMANTIC_LANGUAGE lang);
void bpc_codegen_init_namespace(GList* namespace_chain);
// end call in order

BPC_CODEGEN_TOKEN_ENTRY* bpc_codegen_get_token_entry(const char* token_name);

void bpc_codegen_write_alias(const char* alias_name, BPC_SEMANTIC_BASIC_TYPE basic_t);
void bpc_codegen_write_enum(const char* enum_name, BPC_SEMANTIC_BASIC_TYPE basic_type, GSList* member_list);
void bpc_codegen_write_struct(const char* struct_name, GSList* member_list);
void bpc_codegen_write_msg(const char* msg_name, GSList* member_list);

// call in order
void bpc_codegen_write_opcode();
void bpc_codegen_free_code_file();
// end call in order




void _bpc_codegen_gen_struct_msg_body(const char* token_name, GSList* member_list, bool is_msg);
void _bpc_codegen_copy_template(const char* template_code_file_path);
void _bpc_codegen_free_token_entry(gpointer rawptr);