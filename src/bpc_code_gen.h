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
#define BPCGEN_INDENT_REF (_indent_level)
#define BPCGEN_INDENT_PRINT fputc('\n', _indent_fs); \
for (_indent_loop = UINT32_C(0); _indent_loop < _indent_level; ++_indent_loop) \
fputc('\t', _indent_fs);

/// <summary>
/// the union of struct and msg. because they are almost equal
/// except a little differences. so they share the same write function commonly.
/// </summary>
typedef struct _BPCGEN_STRUCT_LIKE {
	bool is_msg;
	union {
		BPCSMTV_STRUCT* pStruct;
		BPCSMTV_MSG* pMsg;
	}real_ptr;
}BPCGEN_STRUCT_LIKE;

void bpcgen_init_code_file(BPCCMD_PARSED_ARGS* bpc_args);
void bpcgen_write_document(BPCSMTV_DOCUMENT* document);
void bpcgen_free_code_file();

#define BPCGEN_VARTYPE_CONTAIN(val, probe) (val & probe)
typedef enum _BPCGEN_VARTYPE {
	BPCGEN_VARTYPE_NONE = 0,

	BPCGEN_VARTYPE_SINGLE_PRIMITIVE = 1 << 0,
	BPCGEN_VARTYPE_STATIC_PRIMITIVE = 1 << 1,
	BPCGEN_VARTYPE_DYNAMIC_PRIMITIVE = 1 << 2,

	BPCGEN_VARTYPE_SINGLE_STRING = 1 << 3,
	BPCGEN_VARTYPE_STATIC_STRING = 1 << 4,
	BPCGEN_VARTYPE_DYNAMIC_STRING = 1 << 5,

	BPCGEN_VARTYPE_SINGLE_NARROW = 1 << 6,
	BPCGEN_VARTYPE_STATIC_NARROW = 1 << 7,
	BPCGEN_VARTYPE_DYNAMIC_NARROW = 1 << 8,

	BPCGEN_VARTYPE_SINGLE_NATURAL = 1 << 9,
	BPCGEN_VARTYPE_STATIC_NATURAL = 1 << 10,
	BPCGEN_VARTYPE_DYNAMIC_NATURAL = 1 << 11
}BPCGEN_VARTYPE;
typedef struct _BOND_VARS {
	BPCSMTV_VARIABLE** plist_vars;
	BPCGEN_VARTYPE* vars_type;
	
	bool is_bonded;
	uint32_t bond_vars_len;
}BOND_VARS;
/// <summary>
/// construct bond variables
/// </summary>
/// <param name="variables"></param>
/// <param name="bond_rules">the combination of flag enum, BPCGEN_VARTYPE.</param>
/// <returns>A GSList, item is "BOND_VARS*"</returns>
GSList* bpcgen_constructor_bond_vars(GSList* variables, BPCGEN_VARTYPE bond_rules);
/// <summary>
/// free bond variables
/// </summary>
/// <param name="bond_vars"></param>
void bpcgen_destructor_bond_vars(GSList* bond_vars);

/// <summary>
/// pick msg structure from protocol body
/// </summary>
/// <param name="protocol_body">A GSList, item is "BPCSMTV_PROTOCOL_BODY*"</param>
/// <returns>A GSList, item is "BPCSMTV_MSG*"</returns>
GSList* bpcgen_constructor_msg_list(GSList* protocol_body);
/// <summary>
/// free picked msg list safely.
/// </summary>
/// <param name="msg_ls">A GSList, item is "BPCSMTV_MSG*"</param>
void bpcgen_destructor_msg_list(GSList* msg_ls);

/// <summary>
/// an universal function to print enum member for every language output.
/// </summary>
/// <param name="fs"></param>
/// <param name="data"></param>
void bpcgen_print_enum_member(FILE* fs, BPCSMTV_ENUM_MEMBER* data);
/// <summary>
/// print a joined GPtrArray string to file
/// </summary>
/// <param name="fs"></param>
/// <param name="splitter">the splitter, NULL for no splitter.</param>
/// <param name="no_tail">whether the splittor need to be attached to the last item.</param>
/// <param name="parr">a GPtrArray. item is char*</param>
void bpcgen_print_join_ptrarray(FILE* fs, const char* splitter, bool no_tail, GPtrArray* parr);
/// <summary>
/// print a joined GSList string to file
/// </summary>
/// <param name="fs"></param>
/// <param name="splitter">the splitter, NULL for no splitter.</param>
/// <param name="no_tail">whether the splittor need to be attached to the last item.</param>
/// <param name="pslist">a GSList*. item is char*</param>
void bpcgen_print_join_gslist(FILE* fs, const char* splitter, bool no_tail, GSList* pslist);
/// <summary>
/// print annotation for different lang out
/// </summary>
/// <param name="fs"></param>
/// <param name="annotation">the annotation prefix. set to NULL for no prefix</param>
/// <param name="data"></param>
void bpcgen_print_variables_annotation(FILE* fs, const char* annotation, BOND_VARS* data);
/// <summary>
/// pick universal data struct from BPCGEN_STRUCT_LIKE*
/// </summary>
/// <param name="union_data"></param>
/// <param name="is_msg">NULL if you don't need it</param>
/// <param name="variables">NULL if you don't need it</param>
/// <param name="modifier">NULL if you don't need it</param>
/// <param name="struct_like_name">NULL if you don't need it</param>
void bpcgen_pick_struct_like_data(BPCGEN_STRUCT_LIKE* union_data, bool* is_msg, GSList** variables, BPCSMTV_STRUCT_MODIFIER** modifier, char** struct_like_name);


void codepy_write_document(FILE* fs, BPCSMTV_DOCUMENT* document);
void codecs_write_document(FILE* fs, BPCSMTV_DOCUMENT* document);
void codehpp_write_document(FILE* fs, BPCSMTV_DOCUMENT* document);
void codecpp_write_document(FILE* fs, BPCSMTV_DOCUMENT* document, const gchar* hpp_reference);
void codeproto_write_document(FILE* fs, BPCSMTV_DOCUMENT* document);
