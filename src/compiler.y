%require "3.2"

%code requires {
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "bpc_semantic_values.h"
#include "bpc_code_gen.h"
#include "bpc_cmd.h"
#include "bpc_error.h"
#include "bpc_ver.h"
#include "bpc_fs.h"
}

%code provides {
int run_compiler(BPCCMD_PARSED_ARGS* bpc_args);
}

%code {
int yywrap();
void yyerror(const char *s);
void formatted_yyerror(const char* format, ...);

// yyin, yyout and yylex is comes from flex code
// so declare them as extern here
extern FILE* yyin;
extern FILE* yyout;
extern int yylex(void); 

/*
NOTE:
some actions have checked operation and a exception will throw out if check failed.
so, we MUST setup semantic value before any checks, otherwise destructor will work perfectly and
result in memmory leak.
*/

}

%define parse.error detailed
%locations

%union {
	BPCSMTV_BASIC_TYPE token_basic_type;
	char* token_identifier;
	guint64 token_num;
	bool token_bool;

	BPCSMTV_DOCUMENT* nterm_document;
	GSList* nterm_namespace;		// item is char*
	GSList* nterm_define_group;		// item is BPCSMTV_DEFINE_GROUP*

	BPCSMTV_ALIAS* nterm_alias;
	BPCSMTV_ENUM* nterm_enum;
	BPCSMTV_STRUCT* nterm_struct;
	BPCSMTV_MSG* nterm_msg;

	GSList* nterm_enum_body;		// item is BPCSMTV_ENUM_BODY*
	BPCSMTV_ENUM_BODY* nterm_enum_body_entry;
	GSList* nterm_members;			// item is BPCSMTV_MEMBER*
}
%token <token_num> BPC_TOKEN_NUM			"dec_number"
%token <token_identifier> BPC_TOKEN_NAME	"token_name"
%token <token_basic_type> BPC_BASIC_TYPE	"[[u]int[8|16|32|64] | string | double | float]"
%token <token_bool> BPC_RELIABILITY			"[reliable | unreliable]"
%token <token_bool> BPC_FIELD_LAYOUT		"[narrow | nature]"
%token BPC_VERSION			"version"
%token BPC_NAMESPACE		"namespace"
%token BPC_ALIAS			"alias"
%token BPC_ENUM				"enum"
%token BPC_STRUCT			"struct"
%token BPC_MSG				"msg"
%token <token_num> BPC_ARRAY_TUPLE			"[xxx]"
%token BPC_ARRAY_LIST		"[]"
%token BPC_ALIGN			"#xx"
%token BPC_COLON			":"
%token BPC_COMMA			","
%token BPC_SEMICOLON		";"
%token BPC_DOT				"."
%token BPC_LEFT_BRACKET		"{"
%token BPC_RIGHT_BRACKET	"}"
%token BPC_EQUAL			"="

%nterm <nterm_document> bpc_document
%nterm <nterm_namespace> bpc_namespace bpc_namespace_chain
%nterm <nterm_define_group> bpc_define_group
%nterm <nterm_alias> bpc_alias
%nterm <nterm_enum> bpc_enum
%nterm <nterm_struct> bpc_struct
%nterm <nterm_msg> bpc_msg
%nterm <nterm_enum_body> bpc_enum_body
%nterm <nterm_enum_body_entry> bpc_enum_body_entry
%nterm <nterm_members> bpc_member_collection bpc_member_with_align bpc_member_with_array bpc_member

%destructor { bpc_destructor_string($$); } <token_identifier>
%destructor { bpc_destructor_document($$); } <nterm_document>
%destructor { bpc_destructor_string_slist($$); } <nterm_namespace>
%destructor { bpc_destructor_define_group_slist($$); } <nterm_define_group>
%destructor { bpc_destructor_alias($$); } <nterm_alias>
%destructor { bpc_destructor_enum($$); } <nterm_enum>
%destructor { bpc_destructor_struct($$); } <nterm_struct>
%destructor { bpc_destructor_msg($$); } <nterm_msg>
%destructor { bpc_destructor_enum_body_slist($$); } <nterm_enum_body>
%destructor { bpc_destructor_enum_body($$); } <nterm_enum_body_entry>
%destructor { bpc_destructor_member_slist($$); } <nterm_members>

%%

bpc_document:
bpc_version bpc_namespace bpc_define_group
{
	// finish parsing, construct doc
	$$ = bpc_constructor_document();
	$$->namespace_data = $bpc_namespace;
	$$->define_group_data = $bpc_define_group;

	// output code
	bpcgen_write_document($$);

	// free doc
	// WARNING: because bpc_document is a start symbol
	// if we get this symbol, a YYACCEPT will be raised
	// and the destructor declaration will be called automatically
	// so we do not need call it in there now.
	//bpc_destructor_document($$);

	// reset registery
	bpcsmtv_entry_registery_reset();
	bpcsmtv_token_registery_reset();
};

bpc_version:
BPC_VERSION BPC_TOKEN_NUM[sdd_version_num] BPC_SEMICOLON 
{
	if ($sdd_version_num != BPCVER_COMPILER_VERSION) {
		bpc_yyerror("unsupported version: %d. expecting: %d.", $sdd_version_num, BPCVER_COMPILER_VERSION);
		YYABORT;
	}
};

bpc_namespace:
BPC_NAMESPACE bpc_namespace_chain BPC_SEMICOLON 
{
	$$ = $bpc_namespace_chain;
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
	$$ = NULL;
}
|
bpc_define_group[old_define_group] bpc_enum
{
	BPCSMTV_DEFINE_GROUP* gp = bpc_constructor_define_group();
	gp->node_type = BPCSMTV_DEFINED_TOKEN_TYPE_ENUM;
	gp->node_data.enum_data = $bpc_enum;
	$$ = g_slist_append($old_define_group, gp);
}
|
bpc_define_group[old_define_group] bpc_struct
{
	BPCSMTV_DEFINE_GROUP* gp = bpc_constructor_define_group();
	gp->node_type = BPCSMTV_DEFINED_TOKEN_TYPE_STRUCT;
	gp->node_data.struct_data = $bpc_struct;
	$$ = g_slist_append($old_define_group, gp);
}
|
bpc_define_group[old_define_group] bpc_msg
{
	BPCSMTV_DEFINE_GROUP* gp = bpc_constructor_define_group();
	gp->node_type = BPCSMTV_DEFINED_TOKEN_TYPE_MSG;
	gp->node_data.msg_data = $bpc_msg;
	$$ = g_slist_append($old_define_group, gp);
}
|
bpc_define_group[old_define_group] bpc_alias
{
	BPCSMTV_DEFINE_GROUP* gp = bpc_constructor_define_group();
	gp->node_type = BPCSMTV_DEFINED_TOKEN_TYPE_ALIAS;
	gp->node_data.alias_data = $bpc_alias;
	$$ = g_slist_append($old_define_group, gp);
}
|
bpc_define_group[old_define_group] error BPC_RIGHT_BRACKET
{
	$$ = $old_define_group;

	// enum, struct, msg recover
	// we need reset duplication check status
	bpcsmtv_entry_registery_reset();
	yyerrok;	// recover after detecting a BPC_RIGHT_BRACKET
}
|
bpc_define_group[old_define_group] error BPC_SEMICOLON
{
	$$ = $old_define_group;

	// alias recover
	yyerrok;	// recover after detecting a BPC_RIGHT_BRACKET
};

bpc_alias:
BPC_ALIAS BPC_TOKEN_NAME[sdd_user_type] BPC_BASIC_TYPE[sdd_basic_type] BPC_SEMICOLON
{
	$$ = bpc_constructor_alias();
	$$->user_type = $sdd_user_type;
	$$->basic_type = $sdd_basic_type;

	// check token duplication
	if (bpcsmtv_token_registery_test($sdd_user_type)) {
		bpc_yyerror("token %s has been defined.", $sdd_user_type);
		YYERROR;
	}
	bpcsmtv_token_registery_add_alias($$);
};

bpc_enum:
BPC_ENUM BPC_TOKEN_NAME[sdd_enum_name] BPC_COLON BPC_BASIC_TYPE[sdd_enum_type] BPC_LEFT_BRACKET
bpc_enum_body
BPC_RIGHT_BRACKET
{
	$$ = bpc_constructor_enum();
	$$->enum_name = $sdd_enum_name;
	$$->enum_basic_type = $sdd_enum_type;
	$$->enum_body = $bpc_enum_body;

	// check empty body
	if ($bpc_enum_body == NULL) {
		bpc_yyerror("enum %s should have at least 1 entry.", $sdd_enum_name);
		YYERROR;
	}
	// check enum basic type
	if (!bpcsmtv_basic_type_is_suit_for_enum($sdd_enum_type)) {
		bpc_yyerror("the basic type of enum %s is illegal.", $sdd_enum_name);
		YYERROR;
	}
	// check token duplication
	if (bpcsmtv_token_registery_test($sdd_enum_name)) {
		bpc_yyerror("token %s has been defined.", $sdd_enum_name);
		YYERROR;
	}
	bpcsmtv_token_registery_add_enum($$);

	// reset entry check
	bpcsmtv_entry_registery_reset();
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
	BPCSMTV_ENUM_BODY* entry = bpc_constructor_enum_body();
	entry->enum_name = $sdd_name;
	entry->have_specific_value = false;
	entry->specific_value = 0;

	$$ = entry;

	//check entry duplication
	if (bpcsmtv_entry_registery_test($sdd_name)) {
		bpc_yyerror("enum entry %s is duplicated.", $sdd_name);
		YYERROR;
	}
	bpcsmtv_entry_registery_add($sdd_name);
}
|
BPC_TOKEN_NAME[sdd_name] BPC_EQUAL BPC_TOKEN_NUM[sdd_spec_num]
{
	BPCSMTV_ENUM_BODY* entry = bpc_constructor_enum_body();
	entry->enum_name = $sdd_name;
	entry->have_specific_value = true;
	entry->specific_value = $sdd_spec_num;

	$$ = entry;

	//check entry duplication
	if (bpcsmtv_entry_registery_test($sdd_name)) {
		bpc_yyerror("enum entry %s is duplicated.", $sdd_name);
		YYERROR;
	}
	bpcsmtv_entry_registery_add($sdd_name);
};

bpc_struct:
BPC_STRUCT BPC_TOKEN_NAME[sdd_struct_name] BPC_LEFT_BRACKET
bpc_member_collection
BPC_RIGHT_BRACKET
{
	$$ = bpc_constructor_struct();
	$$->struct_name = $sdd_struct_name;
	$$->struct_member = $bpc_member_collection;

	// check token duplication
	if (bpcsmtv_token_registery_test($sdd_struct_name)) {
		bpc_yyerror("token %s has been defined.", $sdd_struct_name);
		YYERROR;
	}
	bpcsmtv_token_registery_add_struct($$);

	// reset entry check
	bpcsmtv_entry_registery_reset();
};

bpc_msg:
BPC_MSG BPC_TOKEN_NAME[sdd_msg_name] BPC_COLON BPC_RELIABLE[sdd_is_reliable] BPC_LEFT_BRACKET
bpc_member_collection
BPC_RIGHT_BRACKET
{
	$$ = bpc_constructor_msg();
	$$->msg_name = $sdd_msg_name;
	$$->is_reliable = $sdd_is_reliable;
	$$->msg_member = $bpc_member_collection;

	// check token duplication
	if (bpcsmtv_token_registery_test($sdd_msg_name)) {
		bpc_yyerror("token %s has been defined.", $sdd_msg_name);
		YYERROR;
	}
	bpcsmtv_token_registery_add_msg($$);

	// reset entry check
	bpcsmtv_entry_registery_reset();
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
	BPCSMTV_MEMBER_ALIGN_PROP data;
	data.use_align = false;
	data.padding_size = 0;

	// apply array prop and move list
	bpcsmtv_member_copy_align_prop($bpc_member_with_array, &data);
	$$ = $bpc_member_with_array;
}
|
bpc_member_with_array BPC_ALIGN BPC_TOKEN_NUM[sdd_extra_size] BPC_SEMICOLON
{
	BPCSMTV_MEMBER_ALIGN_PROP data;
	data.use_align = true;
	data.padding_size = (uint32_t)$sdd_extra_size;

	// apply array prop and move list
	bpcsmtv_member_copy_align_prop($bpc_member_with_array, &data);
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
	BPCSMTV_MEMBER_ARRAY_PROP data;
	data.is_array = false;
	data.is_static_array = false;
	data.array_len = 0u;

	// apply array prop and move list
	bpcsmtv_member_copy_array_prop($bpc_member, &data);
	$$ = $bpc_member;
}
|
bpc_member BPC_ARRAY_TUPLE BPC_TOKEN_NUM[sdd_count]
{
	BPCSMTV_MEMBER_ARRAY_PROP data;
	data.is_array = true;
	data.is_static_array = true;
	data.array_len = (uint32_t)$sdd_count;

	// apply array prop and move list
	bpcsmtv_member_copy_array_prop($bpc_member, &data);
	$$ = $bpc_member;
}
|
bpc_member BPC_ARRAY_LIST
{
	BPCSMTV_MEMBER_ARRAY_PROP data;
	data.is_array = true;
	data.is_static_array = false;
	data.array_len = 0u;

	// apply array prop and move list
	bpcsmtv_member_copy_array_prop($bpc_member, &data);
	$$ = $bpc_member;
};

bpc_member:
BPC_BASIC_TYPE[sdd_basic_type] BPC_TOKEN_NAME[sdd_name]
{
	BPCSMTV_MEMBER* data = bpc_constructor_member();
	data->is_basic_type = true;
	data->v_basic_type = $sdd_basic_type;
	data->vname = $sdd_name;

	$$ = g_slist_append(NULL, data);

	//check entry duplication
	if (bpcsmtv_entry_registery_test($sdd_name)) {
		bpc_yyerror("struct or msg entry %s is duplicated.", $sdd_name);
		YYERROR;
	}
	bpcsmtv_entry_registery_add($sdd_name);

}
|
BPC_TOKEN_NAME[sdd_struct_type] BPC_TOKEN_NAME[sdd_name]
{
	BPCSMTV_MEMBER* data = bpc_constructor_member();
	data->is_basic_type = false;
	data->v_struct_type = $sdd_struct_type;
	data->vname = $sdd_name;

	$$ = g_slist_append(NULL, data);

	//check entry duplication
	if (bpcsmtv_entry_registery_test($sdd_name)) {
		bpc_yyerror("struct or msg entry %s is duplicated.", $sdd_name);
		YYERROR;
	}

	// check token name validation
	BPCSMTV_TOKEN_REGISTERY_ITEM* entry = bpcsmtv_token_registery_get($sdd_struct_type);
	if (entry == NULL || entry->token_type == BPCSMTV_DEFINED_TOKEN_TYPE_MSG) {
		bpc_yyerror("token %s is not defined or it is a msg which should not be specific as a type.", $sdd_struct_type);
		YYERROR;
	}
	bpcsmtv_entry_registery_add($sdd_name);
}
|
bpc_member[sdd_member_chain] BPC_COMMA BPC_TOKEN_NAME[sdd_name]
{
	// duplicate a new one accoding to $sdd_name and the props of previous item
	// add into list
	$$ = bpcsmtv_member_duplicate($sdd_member_chain, $sdd_name);

	//check entry duplication
	if (bpcsmtv_entry_registery_test($sdd_name)) {
		bpc_yyerror("struct or msg entry %s is duplicated.", $sdd_name);
		YYERROR;
	}
	bpcsmtv_entry_registery_add($sdd_name);
};

%%

int yywrap() {
	return 1;
}

void yyerror(const char *s) {
	bpcerr_error(BPCERR_ERROR_SOURCE_PARSER, "%s\n\
	from ln:%d col:%d - ln:%d col:%d", 
		s,
		yylloc.first_line, yylloc.first_column, yylloc.last_line, yylloc.last_column
	);
}

void formatted_yyerror(const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	gchar* buf = bpcfs_vsprintf(format, ap);
	va_end(ap);

	// call real yyerror
	yyerror(buf);

	// free
	g_free(buf);
}

int run_compiler(BPCCMD_PARSED_ARGS* bpc_args) {
	// setup parameters
	yyout = stdout;
	yyin = bpc_args->input_file;
	if (yyin == NULL) {
		fprintf(yyout, "[Error] Fail to open src file.\n");
		return 1;
	}
	// init code file
	bpcgen_init_code_file(bpc_args);

	// do parse
	bpcsmtv_token_registery_reset();
	bpcsmtv_entry_registery_reset();
	int result = yyparse();

	// free resources
	yyin = stdin;
	bpcgen_free_code_file();

	return result;
}
