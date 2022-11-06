#pragma once

#include "bpc_semantic_values.h"
#include "bpc_cmd.h"
#include "snippets.h"
#include "bpc_error.h"
#include "bpc_fs.h"
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>

#define BPCGEN_INDENT_TYPE uint32_t
#define BPCGEN_INDENT_INIT_NEW(fs) uint32_t _indent_level = UINT32_C(0), _indent_loop = UINT32_C(0); FILE* _indent_fs = fs;
#define BPCGEN_INDENT_INIT_REF(fs, ref_indent) uint32_t _indent_level = ref_indent, _indent_loop = 0; FILE* _indent_fs = fs;
#define BPCGEN_INDENT_RESET _indent_level = _indent_loop = UINT32_C(0);
#define BPCGEN_INDENT_INC ++_indent_level;
#define BPCGEN_INDENT_DEC --_indent_level;
#define BPCGEN_INDENT_PRINT fputc('\n', _indent_fs); \
for (_indent_loop = UINT32_C(0); _indent_loop < _indent_level; ++_indent_loop) \
fputc('\t', _indent_fs);

/// <summary>
/// the union of struct and msg. because they are almost equal
/// except a little differences. so they share the same write function commonly.
/// </summary>
typedef union _BPCGEN_STRUCT_LIKE {
	BPCSMTV_STRUCT* pStruct;
	BPCSMTV_MSG* pMsg;
}BPCGEN_STRUCT_LIKE;

void bpcgen_init_code_file(BPCCMD_PARSED_ARGS* bpc_args);
void bpcgen_write_document(BPCSMTV_DOCUMENT* document);
void bpcgen_free_code_file();

///// <summary>
///// pick msg struct from protocol body
///// </summary>
///// <param name="full_list">the original list, item is `BPCSMTV_PROTOCOL_BODY*`</param>
///// <returns>the picked list. item is `BPCSMTV_MSG*` and its items should *not* be free. please free slist directly.</returns>
//GSList* bpcgen_pick_msg_slist(GSList* full_list);

void codepy_write_document(FILE* fs, BPCSMTV_DOCUMENT* document);
void codecs_write_document(FILE* fs, BPCSMTV_DOCUMENT* document);
void codehpp_write_document(FILE* fs, BPCSMTV_DOCUMENT* document);
void codecpp_write_document(FILE* fs, BPCSMTV_DOCUMENT* document, const gchar* hpp_reference);
void codeproto_write_document(FILE* fs, BPCSMTV_DOCUMENT* document);

/*
`like_basic_type` and `underlaying_basic_type` is a pair. both of them usually indicating
whether explicit serialize/deserialize functions or calling serialize/deserialize functions should be used.

`is_pure_basic_type` and `c_style_type` is a pair. both of them always be used in strong-type language, such as C#, C++,
to make code generation more flexible. For example, we need create a field with type `SomeEnum` in C# (enum SomeEnum : UInt32).
Now we need read it, however, we should first dig the basic type of `SomeEnum`, `UInt32` in this example. Then, we need call a type-convertion,
`member = (SomeEnum)br.ReadUInt32()`. In this outputed syntax, we get `SomeEnum` from `c_style_type`, get `UInt32` from `underlaying_basic_type`.

This is the different between these 2 pairs. First pair do a full analyze. The second pair do a partial analyze, it analyze alias but do not analyze enum.
*/

/*

typedef struct _BPCGEN_UNDERLAYING_MEMBER {
	/// <summary>
	/// storage variable delivered by semantic value module
	/// </summary>
	BPCSMTV_MEMBER* semantic_value;

	/// <summary>
	/// Whether field is basic type, after resolving with alias and enum.
	/// When it is false, it mean that this member is must be a struct and
	/// shoule be serialized or deserialized by calling functions rather than
	/// direct code.
	/// </summary>
	bool like_basic_type;
	/// <summary>
	/// The detected basic type of current member.
	/// Only valid when `like_basic_type` is true.
	/// </summary>
	BPCSMTV_BASIC_TYPE underlaying_basic_type;

	/// <summary>
	/// Whether field is basic type, after resolving with alias, without resolving enum.
	/// This bool flag is do a partial resolving comparing with `like_basic_type`.
	/// So if this flag is true, `like_basic_type` must be true.
	/// </summary>
	bool is_pure_basic_type;
	/// <summary>
	/// The c style type of member for convenient code gen.
	/// Only valid when `is_pure_basic_type` is false.
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

*/