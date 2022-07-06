#pragma once

#include "bpc_semantic_values.h"
#include <stdint.h>
#include <stdbool.h>
#include "bpc_cmd.h"

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
void _bpcgen_write_opcode();

//uint32_t _bpc_codegen_get_align_padding_size(BPCSMTV_MEMBER* token);
char* _bpcgen_get_dot_style_namespace(GSList* namespace_list);
void _bpcgen_get_underlaying_type(BPCSMTV_MEMBER* token, bool* pout_proc_like_basic_type, BPCSMTV_BASIC_TYPE* pout_underlaying_basic_type);
void _bpcgen_gen_struct_msg_body(const char* token_name, GSList* member_list, BPCGEN_MSG_EXTRA_PROPS* msg_prop);
void _bpcgen_copy_template(FILE* target, const char* u8_template_code_file_path);
