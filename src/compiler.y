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
}

%code {
int yywrap();
void yyerror(const char *s);
void bpc_yyerror(const char* format, ...);

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
	uint32_t token_num;
	bool token_bool;
	GSList* nterm_namespace;	// item is char*
	GSList* nterm_enum;		// item is char*
	GSList* nterm_members;	// item is BPC_SEMANTIC_MEMBER*
}
%token <token_num> BPC_TOKEN_NUM
%token <token_name> BPC_TOKEN_NAME
%token <token_basic_type> BPC_BASIC_TYPE
%token <token_bool> BPC_RELIABLE
%token BPC_VERSION BPC_NAMESPACE
%token BPC_ALIAS BPC_ENUM BPC_STRUCT BPC_MSG
%token BPC_ARRAY_TUPLE BPC_ARRAY_LIST
%token BPC_ALIGN
%token BPC_COLON BPC_COMMA BPC_SEMICOLON BPC_LEFT_BRACKET BPC_RIGHT_BRACKET BPC_DOT

%nterm <nterm_namespace> bpc_namespace_chain
%nterm <nterm_enum> bpc_enum_body
%nterm <nterm_members> bpc_member_collection bpc_member_with_align bpc_member_with_array bpc_member

%destructor { bpc_destructor_string_slist($$); } <nterm_namespace>
%destructor { bpc_destructor_string_slist($$); } <nterm_enum> 
%destructor { bpc_destructor_semantic_member_slist($$); } <nterm_members>

%%

bpc_document:
bpc_version bpc_namespace bpc_alias_group bpc_define_group
{
	// finish parsing
	bpc_codegen_write_opcode();
};

bpc_version:
BPC_VERSION BPC_TOKEN_NUM[sdd_version_num] BPC_SEMICOLON 
{
	if ($sdd_version_num != BPC_COMPILER_VERSION) {
		bpc_yyerror("[Error] unsupported version: %d. expecting: %d.", $sdd_version_num, BPC_COMPILER_VERSION);
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

bpc_alias_group:
%empty
{
	; //skip
}
|
bpc_alias_group BPC_ALIAS BPC_TOKEN_NAME[sdd_user_type] BPC_BASIC_TYPE[sdd_basic_type] BPC_SEMICOLON
{
	bpc_codegen_write_alias($sdd_user_type, $sdd_basic_type);
	g_free($sdd_user_type);
};

bpc_define_group:
%empty
{
	; //skip
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
};

bpc_enum:
BPC_ENUM BPC_TOKEN_NAME[sdd_enum_name] BPC_COLON BPC_BASIC_TYPE[sdd_enum_type] BPC_LEFT_BRACKET
bpc_enum_body
BPC_RIGHT_BRACKET
{
	// check token duplication
	if (bpc_codegen_get_token_entry($sdd_enum_name) != NULL) {
		bpc_yyerror("[Error] token %s has been defined.", $sdd_enum_name);
		YYERROR;
	}

	// gen code for enum
	bpc_codegen_write_enum($sdd_enum_name, $sdd_enum_type, $bpc_enum_body);
	// and free name and member list
	g_free($sdd_enum_name);
	bpc_destructor_string_slist($bpc_enum_body);
};

bpc_enum_body:
BPC_TOKEN_NAME[sdd_name]
{
	$$ = g_slist_append(NULL, $sdd_name);
}
|
bpc_enum_body[sdd_enum_chain] BPC_COMMA BPC_TOKEN_NAME[sdd_name]
{
	$$ = g_slist_append($sdd_enum_chain, $sdd_name);
};

bpc_struct:
BPC_STRUCT BPC_TOKEN_NAME[sdd_struct_name] BPC_LEFT_BRACKET
bpc_member_collection
BPC_RIGHT_BRACKET
{
	// check token duplication
	if (bpc_codegen_get_token_entry($sdd_struct_name) != NULL) {
		bpc_yyerror("[Error] token %s has been defined.", $sdd_struct_name);
		YYERROR;
	}

	// gen code for struct
	bpc_codegen_write_struct($sdd_struct_name, $bpc_member_collection);
	// and free name and member list
	g_free($sdd_struct_name);
	bpc_destructor_semantic_member_slist($bpc_member_collection);
};

bpc_msg:
BPC_MSG BPC_TOKEN_NAME[sdd_msg_name] BPC_COLON BPC_RELIABLE[sdd_is_reliable] BPC_LEFT_BRACKET
bpc_member_collection
BPC_RIGHT_BRACKET
{
	// check token duplication
	if (bpc_codegen_get_token_entry($sdd_msg_name) != NULL) {
		bpc_yyerror("[Error] token %s has been defined.", $sdd_msg_name);
		YYERROR;
	}

	// gen code for msg
	bpc_codegen_write_msg($sdd_msg_name, $bpc_member_collection, $sdd_is_reliable);
	// and free name and member list
	g_free($sdd_msg_name);
	bpc_destructor_semantic_member_slist($bpc_member_collection);
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
	BPC_SEMANTIC_MEMBER* data = bpc_constructor_semantic_member();
	data->is_basic_type = true;
	data->v_basic_type = $sdd_basic_type;
	data->vname = $sdd_name;

	$$ = g_slist_append(NULL, data);
}
|
BPC_TOKEN_NAME[sdd_struct_type] BPC_TOKEN_NAME[sdd_name]
{
	// check token name validation
	BPC_CODEGEN_TOKEN_ENTRY* entry = bpc_codegen_get_token_entry($sdd_struct_type);
	if (entry == NULL || entry->token_type == BPC_CODEGEN_TOKEN_TYPE_MSG) {
		bpc_yyerror("[Error] token %s is not defined.", $sdd_struct_type);
		YYERROR;
	}

	BPC_SEMANTIC_MEMBER* data = bpc_constructor_semantic_member();
	data->is_basic_type = false;
	data->v_struct_type = $sdd_struct_type;
	data->vname = $sdd_name;

	$$ = g_slist_append(NULL, data);
}
|
bpc_member[sdd_member_chain] BPC_COMMA BPC_TOKEN_NAME[sdd_name]
{
	// get a references item from old chain. old chain have at least item so 
	// this oper is safe.
	GSList* formed_member_chain = (GSList*)$sdd_member_chain;
	BPC_SEMANTIC_MEMBER* references_item = (BPC_SEMANTIC_MEMBER*)formed_member_chain->data;

	// construct new item and apply some properties from
	// references item
	BPC_SEMANTIC_MEMBER* data = bpc_constructor_semantic_member();
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
	printf("[Error]     from ln:%d col:%d - ln:%d col:%d\n", 
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

		fclose(yyin);
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
