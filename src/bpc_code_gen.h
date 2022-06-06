#pragma once

#include "bpc_semantic_values.h"

// call in order
void init_code_file(const char* filepath);
void init_language(BPC_SEMANTIC_LANGUAGE lang);
void init_namespace(GList* namespace_chain);
// end call in order

void write_alias(const char* alias_name, BPC_SEMANTIC_BASIC_TYPE basic_t);
void write_enum(const char* enum_name, BPC_SEMANTIC_BASIC_TYPE basic_type, GList* member_list);
void write_struct(const char* struct_name, GList* member_list);
void write_msg(const char* msg_name, GList* member_list);

// call in order
void write_opcode();
void free_code_file();
// end call in order
