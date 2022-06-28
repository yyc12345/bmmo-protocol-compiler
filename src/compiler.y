%require "3.2"

%code requires {
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "bpc_semantic_values.h"
#include "bpc_code_gen.h"
#include "bpc_cmd.h"
}

%code provides {
int run_compiler(BPC_CMD_PARSED_ARGS* bpc_args);
void bpc_yyerror(const char* format, ...);
}

%code {
int yywrap();
void yyerror(const char *s);

// yyin, yyout and yylex is comes from flex code
// so declare them as extern here
extern FILE* yyin;
extern FILE* yyout;
extern int yylex(void); 

}

%define parse.error detailed
%locations

%union {
	BPC_SEMANTIC_BASIC_TYPE token_basic_type;
	char* token_name;
	int64_t token_num;
	bool token_bool;
	GSList* nterm_namespace;	// item is char*
	GSList* nterm_enum;		// item is BPC_SEMANTIC_ENUM_BODY*
	BPC_SEMANTIC_ENUM_BODY* nterm_enum_entry;
	GSList* nterm_members;	// item is BPC_SEMANTIC_MEMBER*
}
%token <token_num> BPC_TOKEN_NUM			"dec_number"
%token <token_name> BPC_TOKEN_NAME			"token_name"
%token <token_basic_type> BPC_BASIC_TYPE	"[[u]int[8|16|32|64] | string | double | float]"
%token <token_bool> BPC_RELIABLE			"[reliable | unreliable]"
%token BPC_VERSION			"version"
%token BPC_NAMESPACE		"namespace"
%token BPC_ALIAS			"alias"
%token BPC_ENUM				"enum"
%token BPC_STRUCT			"struct"
%token BPC_MSG				"msg"
%token BPC_ARRAY_TUPLE		"tuple"
%token BPC_ARRAY_LIST		"list"
%token BPC_ALIGN			"align"
%token BPC_COLON			":"
%token BPC_COMMA			","
%token BPC_SEMICOLON		";"
%token BPC_LEFT_BRACKET		"{"
%token BPC_RIGHT_BRACKET	"}"
%token BPC_DOT				"."
%token BPC_EQUAL			"="

%nterm <nterm_namespace> bpc_namespace_chain
%nterm <nterm_enum> bpc_enum_body
%nterm <nterm_enum_entry> bpc_enum_body_entry
%nterm <nterm_members> bpc_member_collection bpc_member_with_align bpc_member_with_array bpc_member

%destructor { bpc_destructor_string($$); } <token_name>
%destructor { bpc_destructor_string_slist($$); } <nterm_namespace>
%destructor { bpc_destructor_enum_body_slist($$); } <nterm_enum>
%destructor { bpc_destructor_enum_body($$); } <nterm_enum_entry>
%destructor { bpc_destructor_member_slist($$); } <nterm_members>

%%

bpc_document:
bpc_version bpc_namespace bpc_define_group
{
	// finish parsing
	bpc_codegen_write_opcode();
};

bpc_version:
BPC_VERSION BPC_TOKEN_NUM[sdd_version_num] BPC_SEMICOLON 
{
	if ($sdd_version_num != BPC_COMPILER_VERSION) {
		bpc_yyerror("unsupported version: %d. expecting: %d.", $sdd_version_num, BPC_COMPILER_VERSION);
		YYABORT;
	}
};

bpc_namespace:
BPC_NAMESPACE bpc_namespace_chain BPC_SEMICOLON 
{
	// init namespace
	bpc_codegen_init_namespace($bpc_namespace_chain);
	// and free namespace chain
	bpc_destructor_string_slist($bpc_namespace_chain);
};


bpc_namespace_chain:
BPC_TOKEN_NAME[sdd_name]
{
	$$ = g_slist_append(NULL, $sdd_name);
}
|
bpc_namespace_chain[sdd_old_chain] BPC_DOT BPC_TOKEN_NAME[sdd_name]
{
	$$ = g_slist_append($sdd_old_chain, $sdd_name);
};

bpc_define_group:
%empty
{
	// we need reset duplication check status
	bpc_semantic_check_duplication_reset();
}
|
bpc_define_group bpc_enum
{
	; //skip
}
|
bpc_define_group bpc_struct
{
	; //skip
}
|
bpc_define_group bpc_msg
{
	; //skip
}
|
bpc_define_group bpc_alias
{
	; //skip
}
|
bpc_define_group error BPC_RIGHT_BRACKET
{
	// enum, struct, msg recover
	// we need reset duplication check status
	bpc_semantic_check_duplication_reset();
	yyerrok;	// recover after detecting a BPC_RIGHT_BRACKET
}
|
bpc_define_group error BPC_SEMICOLON
{
	// alias recover
	yyerrok;	// recover after detecting a BPC_RIGHT_BRACKET
};

bpc_alias:
BPC_ALIAS BPC_TOKEN_NAME[sdd_user_type] BPC_BASIC_TYPE[sdd_basic_type] BPC_SEMICOLON
{
	// check token duplication
	if (bpc_codegen_get_token_entry($sdd_user_type) != NULL) {
		bpc_yyerror("token %s has been defined.", $sdd_user_type);
		YYERROR;
	}

	bpc_codegen_write_alias($sdd_user_type, $sdd_basic_type);
	g_free($sdd_user_type);
};

bpc_enum:
BPC_ENUM BPC_TOKEN_NAME[sdd_enum_name] BPC_COLON BPC_BASIC_TYPE[sdd_enum_type] BPC_LEFT_BRACKET
bpc_enum_body
BPC_RIGHT_BRACKET
{
	// check token duplication
	if (bpc_codegen_get_token_entry($sdd_enum_name) != NULL) {
		bpc_yyerror("token %s has been defined.", $sdd_enum_name);
		YYERROR;
	}
	// check empty body
	if ($bpc_enum_body == NULL) {
		bpc_yyerror("enum %s should have at least 1 entry.", $sdd_enum_name);
		YYERROR;
	}

	// gen code for enum
	bpc_codegen_write_enum($sdd_enum_name, $sdd_enum_type, $bpc_enum_body);
	// and free name and member list
	g_free($sdd_enum_name);
	bpc_destructor_enum_body_slist($bpc_enum_body);
	bpc_semantic_check_duplication_reset();
};

bpc_enum_body:
bpc_enum_body_entry
{
	$$ = g_slist_append(NULL, $bpc_enum_body_entry);
}
|
bpc_enum_body[sdd_enum_chain] BPC_COMMA bpc_enum_body_entry
{
	$$ = g_slist_append($sdd_enum_chain, $bpc_enum_body_entry);
};

bpc_enum_body_entry:
BPC_TOKEN_NAME[sdd_name]
{
	//check entry duplication
	if (bpc_semantic_check_duplication($sdd_name)) {
		bpc_yyerror("enum entry %s is duplicated.", $sdd_name);
		YYERROR;
	}

	BPC_SEMANTIC_ENUM_BODY* entry = bpc_constructor_enum_body();
	entry->enum_name = $sdd_name;
	entry->have_specific_value = false;
	entry->specific_value = 0;

	$$ = entry;
}
|
BPC_TOKEN_NAME[sdd_name] BPC_EQUAL BPC_TOKEN_NUM[sdd_spec_num]
{
	//check entry duplication
	if (bpc_semantic_check_duplication($sdd_name)) {
		bpc_yyerror("enum entry %s is duplicated.", $sdd_name);
		YYERROR;
	}

	BPC_SEMANTIC_ENUM_BODY* entry = bpc_constructor_enum_body();
	entry->enum_name = $sdd_name;
	entry->have_specific_value = true;
	entry->specific_value = $sdd_spec_num;

	$$ = entry;
};

bpc_struct:
BPC_STRUCT BPC_TOKEN_NAME[sdd_struct_name] BPC_LEFT_BRACKET
bpc_member_collection
BPC_RIGHT_BRACKET
{
	// check token duplication
	if (bpc_codegen_get_token_entry($sdd_struct_name) != NULL) {
		bpc_yyerror("token %s has been defined.", $sdd_struct_name);
		YYERROR;
	}

	// gen code for struct
	bpc_codegen_write_struct($sdd_struct_name, $bpc_member_collection);
	// and free name and member list
	g_free($sdd_struct_name);
	bpc_destructor_member_slist($bpc_member_collection);
	bpc_semantic_check_duplication_reset();
};

bpc_msg:
BPC_MSG BPC_TOKEN_NAME[sdd_msg_name] BPC_COLON BPC_RELIABLE[sdd_is_reliable] BPC_LEFT_BRACKET
bpc_member_collection
BPC_RIGHT_BRACKET
{
	// check token duplication
	if (bpc_codegen_get_token_entry($sdd_msg_name) != NULL) {
		bpc_yyerror("token %s has been defined.", $sdd_msg_name);
		YYERROR;
	}

	// gen code for msg
	bpc_codegen_write_msg($sdd_msg_name, $bpc_member_collection, $sdd_is_reliable);
	// and free name and member list
	g_free($sdd_msg_name);
	bpc_destructor_member_slist($bpc_member_collection);
	bpc_semantic_check_duplication_reset();
};

bpc_member_collection:
%empty
{
	$$ = NULL;
}
|
bpc_member_collection[sdd_old_members] bpc_member_with_align
{
	$$ = g_slist_concat($sdd_old_members, $bpc_member_with_align);
};

bpc_member_with_align:
bpc_member_with_array BPC_SEMICOLON
{
	BPC_SEMANTIC_MEMBER_ALIGN_PROP data;
	data.use_align = false;
	data.padding_size = 0;

	// apply array prop and move list
	g_slist_foreach($bpc_member_with_array, bpc_lambda_semantic_member_copy_align_prop, &data);
	$$ = $bpc_member_with_array;
}
|
bpc_member_with_array BPC_ALIGN BPC_TOKEN_NUM[sdd_expected_size] BPC_SEMICOLON
{
	BPC_SEMANTIC_MEMBER_ALIGN_PROP data;
	data.use_align = true;
	data.padding_size = (uint32_t)$sdd_expected_size;

	// apply array prop and move list
	g_slist_foreach($bpc_member_with_array, bpc_lambda_semantic_member_copy_align_prop, &data);
	$$ = $bpc_member_with_array;
}
|
error BPC_SEMICOLON
{
	$$ = NULL;
	yyerrok;	// recover after detecting a BPC_SEMICOLON
};

bpc_member_with_array:
bpc_member
{
	BPC_SEMANTIC_MEMBER_ARRAY_PROP data;
	data.is_array = false;
	data.is_static_array = false;
	data.array_len = 0u;

	// apply array prop and move list
	g_slist_foreach($bpc_member, bpc_lambda_semantic_member_copy_array_prop, &data);
	$$ = $bpc_member;
}
|
bpc_member BPC_ARRAY_TUPLE BPC_TOKEN_NUM[sdd_count]
{
	BPC_SEMANTIC_MEMBER_ARRAY_PROP data;
	data.is_array = true;
	data.is_static_array = true;
	data.array_len = (uint32_t)$sdd_count;

	// apply array prop and move list
	g_slist_foreach($bpc_member, bpc_lambda_semantic_member_copy_array_prop, &data);
	$$ = $bpc_member;
}
|
bpc_member BPC_ARRAY_LIST
{
	BPC_SEMANTIC_MEMBER_ARRAY_PROP data;
	data.is_array = true;
	data.is_static_array = false;
	data.array_len = 0u;

	// apply array prop and move list
	g_slist_foreach($bpc_member, bpc_lambda_semantic_member_copy_array_prop, &data);
	$$ = $bpc_member;
};

bpc_member:
BPC_BASIC_TYPE[sdd_basic_type] BPC_TOKEN_NAME[sdd_name]
{
	//check entry duplication
	if (bpc_semantic_check_duplication($sdd_name)) {
		bpc_yyerror("struct or msg entry %s is duplicated.", $sdd_name);
		YYERROR;
	}

	BPC_SEMANTIC_MEMBER* data = bpc_constructor_member();
	data->is_basic_type = true;
	data->v_basic_type = $sdd_basic_type;
	data->vname = $sdd_name;

	$$ = g_slist_append(NULL, data);
}
|
BPC_TOKEN_NAME[sdd_struct_type] BPC_TOKEN_NAME[sdd_name]
{
	//check entry duplication
	if (bpc_semantic_check_duplication($sdd_name)) {
		bpc_yyerror("struct or msg entry %s is duplicated.", $sdd_name);
		YYERROR;
	}

	// check token name validation
	BPC_CODEGEN_TOKEN_ENTRY* entry = bpc_codegen_get_token_entry($sdd_struct_type);
	if (entry == NULL || entry->token_type == BPC_CODEGEN_TOKEN_TYPE_MSG) {
		bpc_yyerror("token %s is not defined.", $sdd_struct_type);
		YYERROR;
	}

	BPC_SEMANTIC_MEMBER* data = bpc_constructor_member();
	data->is_basic_type = false;
	data->v_struct_type = $sdd_struct_type;
	data->vname = $sdd_name;

	$$ = g_slist_append(NULL, data);
}
|
bpc_member[sdd_member_chain] BPC_COMMA BPC_TOKEN_NAME[sdd_name]
{
	//check entry duplication
	if (bpc_semantic_check_duplication($sdd_name)) {
		bpc_yyerror("struct or msg entry %s is duplicated.", $sdd_name);
		YYERROR;
	}

	// get a references item from old chain. old chain have at least item so 
	// this oper is safe.
	GSList* formed_member_chain = (GSList*)$sdd_member_chain;
	BPC_SEMANTIC_MEMBER* references_item = (BPC_SEMANTIC_MEMBER*)formed_member_chain->data;

	// construct new item and apply some properties from
	// references item
	BPC_SEMANTIC_MEMBER* data = bpc_constructor_member();
	data->vname = $sdd_name;
	data->is_basic_type = references_item->is_basic_type;
	if (data->is_basic_type) {
		data->v_basic_type = references_item->v_basic_type;
	} else {
		// use strdup to ensure each item have unique string in memory
		// for safely free memory for each item.
		data->v_struct_type = g_strdup(references_item->v_struct_type);
	}

	// add into list
	$$ = g_slist_append(formed_member_chain, data);
};

%%

int yywrap() {
	return 1;
}

void yyerror(const char *s) {
	printf("[Error] %s\n", s);
	printf("            from ln:%d col:%d - ln:%d col:%d\n", 
		yylloc.first_line, yylloc.first_column, yylloc.last_line, yylloc.last_column);
}

void bpc_yyerror(const char* format, ...) {
	// generate format string
	GString* buf = g_string_new(NULL);
	va_list ap;
	va_start(ap, format);
	g_string_vprintf(buf, format, ap);
	va_end(ap);

	// call real yyerror
	yyerror(buf->str);

	// free
	g_string_free(buf, TRUE);
}

int run_compiler(BPC_CMD_PARSED_ARGS* bpc_args) {
	// setup parameters
	yyout = stdout;
	yyin = bpc_args->input_file;
	if (yyin == NULL) {
		fprintf(yyout, "[Error] Fail to open src file.\n");
		return 1;
	}
	if (!bpc_codegen_init_code_file(bpc_args)) {
		fprintf(yyout, "[Error] Fail to open dest file.\n");

		yyin = stdin;
		return 1;
	}

	// do parse
	int result = yyparse();

	// free resources
	fclose(yyin);
	yyin = stdin;
	bpc_codegen_free_code_file();

	return result;
}
