%require "3.2"

%code requires {
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "bpc_semantic_values.h"
#include "bpc_code_gen.h"
#include "bpc_cmd.h"
#include "bpc_error.h"
#include "bpc_ver.h"
#include "bpc_fs.h"
}

%code provides {
typedef void (*bpcyy_compiler_test_probe) (BPCSMTV_DOCUMENT* document);
int bpcyy_run_compiler(BPCCMD_PARSED_ARGS* bpc_args, bpcyy_compiler_test_probe probe);
}

%code {
int yywrap();
void yyerror(const char *s);
void bpcyy_error_fmt(const char* format, ...);

static bpcyy_compiler_test_probe test_probe = NULL;

// yyin, yyout and yylex is comes from flex code
// so declare them as extern here
extern FILE* yyin;
extern FILE* yyout;
extern int yylex(void); 

/*
NOTE:
some actions have checked operation and a exception will throw out if check failed.
so, we MUST setup semantic value NULL first.
after essential check, we can set real data of it. at the same time, all destructor need do NULL check.
otherwise, a memmory leak will be raised.
*/

}

%define parse.error detailed
%locations

%union {
	BPCSMTV_BASIC_TYPE token_basic_type;
	char* token_identifier;
	BPCSMTV_COMPOUND_NUMBER token_num;
	bool token_bool;

	BPCSMTV_DOCUMENT* nterm_document;
	GSList* nterm_namespace;			// item is char*
	GSList* nterm_protocol_body;		// item is BPCSMTV_PROTOCOL_BODY*

	BPCSMTV_ALIAS* nterm_alias;
	BPCSMTV_ENUM* nterm_enum;
	BPCSMTV_STRUCT* nterm_struct;
	BPCSMTV_MSG* nterm_msg;

	BPCSMTV_ENUM_MEMBER* nterm_enum_member;
	GSList* nterm_enum_body;			// item is BPCSMTV_ENUM_MEMBER*
	
	BPCSMTV_STRUCT_MODIFIER* nterm_struct_modifier;
	BPCSMTV_VARIABLE_ARRAY* nterm_variable_array;
	BPCSMTV_VARIABLE_TYPE* nterm_variable_type;
	BPCSMTV_VARIABLE_ALIGN* nterm_variable_align;
	GSList* nterm_variable_declarators;	// item is char*
	GSList* nterm_variables;			// item is BPCSMTV_VARIABLE*
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
%token <token_num> BPC_ALIGN				"#xx"
%token BPC_COMMA			","
%token BPC_SEMICOLON		";"
%token BPC_DOT				"."
%token BPC_LEFT_BRACKET		"{"
%token BPC_RIGHT_BRACKET	"}"
%token BPC_EQUAL			"="

%nterm <nterm_document> bpc_document
%nterm <nterm_namespace> bpc_namespace bpc_namespace_chain
%nterm <nterm_protocol_body> bpc_protocol_body
%nterm <nterm_alias> bpc_alias
%nterm <nterm_enum> bpc_enum
%nterm <nterm_struct> bpc_struct
%nterm <nterm_msg> bpc_msg
%nterm <nterm_enum_body> bpc_enum_body
%nterm <nterm_enum_member> bpc_enum_member
%nterm <nterm_struct_modifier> bpc_struct_modifier
%nterm <nterm_variable_array> bpc_variable_array
%nterm <nterm_variable_type> bpc_variable_type
%nterm <nterm_variable_align> bpc_variable_align
%nterm <nterm_variable_declarators> bpc_variable_declarators
%nterm <nterm_variables> bpc_variables

%destructor { bpcsmtv_destructor_string($$); } <token_identifier>
%destructor { bpcsmtv_destructor_document($$); } <nterm_document>
%destructor { bpcsmtv_destructor_slist_string($$); } <nterm_namespace>
%destructor { bpcsmtv_destructor_slist_protocol_body($$); } <nterm_protocol_body>
%destructor { bpcsmtv_destructor_alias($$); } <nterm_alias>
%destructor { bpcsmtv_destructor_enum($$); } <nterm_enum>
%destructor { bpcsmtv_destructor_struct($$); } <nterm_struct>
%destructor { bpcsmtv_destructor_msg($$); } <nterm_msg>
%destructor { bpcsmtv_destructor_slist_enum_member($$); } <nterm_enum_body>
%destructor { bpcsmtv_destructor_enum_member($$); } <nterm_enum_member>
%destructor { bpcsmtv_destructor_struct_modifier($$); } <nterm_struct_modifier>
%destructor { bpcsmtv_destructor_variable_array($$); } <nterm_variable_array>
%destructor { bpcsmtv_destructor_variable_type($$); } <nterm_variable_type>
%destructor { bpcsmtv_destructor_variable_align($$); } <nterm_variable_align>
%destructor { bpcsmtv_destructor_slist_string($$); } <nterm_variable_declarators>
%destructor { bpcsmtv_destructor_slist_variable($$); } <nterm_variables>

%%

bpc_document:
bpc_version bpc_namespace bpc_protocol_body
{
	// finish parsing, construct doc
	$$ = bpcsmtv_constructor_document();
	$$->namespace_data = $bpc_namespace;
	$$->protocol_body = $bpc_protocol_body;

	// output code
	// only generate code when no thrown error
	bpcgen_write_document($$);

	// free doc
	// WARNING: because bpc_document is a start symbol
	// if we get this symbol, a YYACCEPT will be raised
	// and the destructor declaration will be called automatically
	// so we do not need call it in there now.
	//bpcsmtv_destructor_document($$);

	// run test probe if necessary
	// probe should be run before register free, because probe need register data
	if (test_probe != NULL) {
		(*test_probe)($$);
	}

	// reset all registery
	bpcsmtv_registery_identifier_reset();
	bpcsmtv_registery_variables_reset();
};

bpc_version:
BPC_VERSION BPC_TOKEN_NUM[sdd_version_num] BPC_SEMICOLON
{
	// check number first
	uint32_t parsed = UINT32_C(0);
	if (!bpcsmtv_get_offset_number(&($sdd_version_num), &parsed)) {
		bpcyy_error_fmt("Invalid number for bpc version.");
		YYABORT;
	}
	// check version
	if (parsed != BPCVER_COMPILER_VERSION) {
		bpcyy_error_fmt("Unsupported version: %" PRIu32 ". Expecting: %" PRIu32 ".", 
			parsed, BPCVER_COMPILER_VERSION);
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
bpc_namespace_chain[sdd_parent_chain] BPC_DOT BPC_TOKEN_NAME[sdd_name]
{
	$$ = g_slist_append($sdd_parent_chain, $sdd_name);
};

bpc_protocol_body:
%empty
{
	$$ = NULL;
	bpcsmtv_registery_variables_reset();
}
|
bpc_protocol_body[sdd_parent_protocol_body] bpc_alias
{
	BPCSMTV_PROTOCOL_BODY* node = bpcsmtv_constructor_protocol_body();
	node->node_type = BPCSMTV_DEFINED_IDENTIFIER_TYPE_ALIAS;
	node->node_data.alias_data = $bpc_alias;
	$$ = g_slist_append($sdd_parent_protocol_body, node);

	bpcsmtv_registery_identifier_add(node);
	bpcsmtv_registery_variables_reset();
}
|
bpc_protocol_body[sdd_parent_protocol_body] bpc_enum
{
	BPCSMTV_PROTOCOL_BODY* node = bpcsmtv_constructor_protocol_body();
	node->node_type = BPCSMTV_DEFINED_IDENTIFIER_TYPE_ENUM;
	node->node_data.enum_data = $bpc_enum;
	$$ = g_slist_append($sdd_parent_protocol_body, node);

	bpcsmtv_registery_identifier_add(node);
	bpcsmtv_registery_variables_reset();
}
|
bpc_protocol_body[sdd_parent_protocol_body] bpc_struct
{
	BPCSMTV_PROTOCOL_BODY* node = bpcsmtv_constructor_protocol_body();
	node->node_type = BPCSMTV_DEFINED_IDENTIFIER_TYPE_STRUCT;
	node->node_data.struct_data = $bpc_struct;
	$$ = g_slist_append($sdd_parent_protocol_body, node);

	bpcsmtv_registery_identifier_add(node);
	bpcsmtv_registery_variables_reset();
}
|
bpc_protocol_body[sdd_parent_protocol_body] bpc_msg
{
	BPCSMTV_PROTOCOL_BODY* node = bpcsmtv_constructor_protocol_body();
	node->node_type = BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG;
	node->node_data.msg_data = $bpc_msg;
	$$ = g_slist_append($sdd_parent_protocol_body, node);

	bpcsmtv_registery_identifier_add(node);
	bpcsmtv_registery_variables_reset();
}
|
bpc_protocol_body[sdd_parent_protocol_body] error BPC_RIGHT_BRACKET
{
	$$ = $sdd_parent_protocol_body;
	bpcsmtv_registery_variables_reset();

	// enum, struct, msg recover
	yyerrok;	// recover after detecting a BPC_RIGHT_BRACKET
}
|
bpc_protocol_body[sdd_parent_protocol_body] error BPC_SEMICOLON
{
	$$ = $sdd_parent_protocol_body;
	bpcsmtv_registery_variables_reset();

	// alias recover
	yyerrok;	// recover after detecting a BPC_RIGHT_BRACKET
};

bpc_alias:
BPC_ALIAS BPC_TOKEN_NAME[sdd_user_type] BPC_BASIC_TYPE[sdd_basic_type] BPC_SEMICOLON
{
	$$ = NULL;
	// check identifier
	if (bpcsmtv_registery_identifier_test($sdd_user_type)) {
		bpcyy_error_fmt("Duplicated identifier: \"%s\"", $sdd_user_type);
		YYERROR;
	}

	// set data
	$$ = bpcsmtv_constructor_alias();
	$$->custom_type = $sdd_user_type;
	$$->basic_type = $sdd_basic_type;
};

bpc_enum:
BPC_ENUM BPC_BASIC_TYPE[sdd_enum_type] BPC_TOKEN_NAME[sdd_enum_name] BPC_LEFT_BRACKET
bpc_enum_body
BPC_RIGHT_BRACKET
{
	$$ = NULL;
	// check basic type
	if (!bpcsmtv_is_basic_type_suit_for_enum($sdd_enum_type)) {
		bpcyy_error_fmt("Specified basic type is not suit for enum \"%s\".", $sdd_enum_name);
		YYERROR;
	}
	// check identifier
	if (bpcsmtv_registery_identifier_test($sdd_enum_name)) {
		bpcyy_error_fmt("Duplicated identifier: \"%s\"", $sdd_enum_name);
		YYERROR;
	}
	// check body limit
	// distribute value and check value legality at the same time.
	if (!bpcsmtv_arrange_enum_body_value($bpc_enum_body, $sdd_enum_type, $sdd_enum_name)) {
		bpcyy_error_fmt("Enum \"%s\" is invalid because its members have some issues which have been introduced above.", $sdd_enum_name);
		YYERROR;
	}

	// set data
	// enum body has been processed, so we do not process it again.
	$$ = bpcsmtv_constructor_enum();
	$$->enum_name = $sdd_enum_name;
	$$->enum_basic_type = $sdd_enum_type;
	$$->enum_body = $bpc_enum_body;
};

bpc_enum_body:
bpc_enum_member
{
	$$ = g_slist_append(NULL, $bpc_enum_member);
}
|
bpc_enum_body[sdd_parent_enum_body] BPC_COMMA bpc_enum_member
{
	$$ = g_slist_append($sdd_parent_enum_body, $bpc_enum_member);
};

bpc_enum_member:
BPC_TOKEN_NAME[sdd_name]
{
	$$ = NULL;
	// check variable duplication first
	if (bpcsmtv_registery_variables_test($sdd_name)) {
		bpcyy_error_fmt("Duplicated enum member \"%s\"", $sdd_name);
		YYERROR;
	}

	$$ = bpcsmtv_constructor_enum_member();
	$$->enum_member_name = $sdd_name;
	bpcsmtv_assign_enum_member_value($$, NULL);
	
	bpcsmtv_registery_variables_add($sdd_name);
}
|
BPC_TOKEN_NAME[sdd_name] BPC_EQUAL BPC_TOKEN_NUM[sdd_spec_num]
{
	$$ = NULL;
	// check variable duplication first
	if (bpcsmtv_registery_variables_test($sdd_name)) {
		bpcyy_error_fmt("Duplicated enum member \"%s\"", $sdd_name);
		YYERROR;
	}

	$$ = bpcsmtv_constructor_enum_member();
	$$->enum_member_name = $sdd_name;
	bpcsmtv_assign_enum_member_value($$, &($sdd_spec_num));
	
	bpcsmtv_registery_variables_add($sdd_name);
};

bpc_struct_modifier:
%empty
{
	$$ = bpcsmtv_constructor_struct_modifier();
	$$->has_set_reliability = false;
	$$->is_reliable = true;
	$$->has_set_field_layout = false;
	$$->is_narrow = true;
	$$->struct_size = UINT32_C(1);
	$$->struct_unit_size = UINT32_C(1);
}
|
bpc_struct_modifier[sdd_parent_modifier] BPC_RELIABILITY[sdd_reliability]
{
	$$ = NULL;
	// check duplicated assign
	if ($sdd_parent_modifier->has_set_reliability) {
		bpcyy_error_fmt("Duplicated assignment of reliability modifier.");
		YYERROR;
	}

	// set
	$$ = $sdd_parent_modifier;
	$$->has_set_reliability = true;
	$$->is_reliable = $sdd_reliability;
}
|
bpc_struct_modifier[sdd_parent_modifier] BPC_FIELD_LAYOUT[sdd_field_layout]
{
	$$ = NULL;
	// check duplicated assign
	if ($sdd_parent_modifier->has_set_field_layout) {
		bpcyy_error_fmt("Duplicated assignment of field layout modifier.");
		YYERROR;
	}

	// set
	$$ = $sdd_parent_modifier;
	$$->has_set_field_layout = true;
	$$->is_narrow = $sdd_field_layout;
};

bpc_struct:
bpc_struct_modifier BPC_STRUCT BPC_TOKEN_NAME[sdd_struct_name] BPC_LEFT_BRACKET
bpc_variables
BPC_RIGHT_BRACKET
{
	$$ = NULL;
	// check modifier
	if (!bpcsmtv_is_modifier_suit_struct($bpc_struct_modifier)) {
		bpcyy_error_fmt("Invalid struct modifier of \"%s\". Reliability modifier only can be applied for Msg define.", $sdd_struct_name);
		YYERROR;
	}
	// check identifier
	if (bpcsmtv_registery_identifier_test($sdd_struct_name)) {
		bpcyy_error_fmt("Duplicated identifier: \"%s\"", $sdd_struct_name);
		YYERROR;
	}

	// set data
	// setup field layout
	bpcsmtv_setup_field_layout($bpc_variables, $bpc_struct_modifier, $sdd_struct_name);
	// fill data
	$$ = bpcsmtv_constructor_struct();
	$$->struct_modifier = $bpc_struct_modifier;
	$$->struct_name = $sdd_struct_name;
	$$->struct_body = $bpc_variables;
};

bpc_msg:
bpc_struct_modifier BPC_MSG BPC_TOKEN_NAME[sdd_msg_name] BPC_LEFT_BRACKET
bpc_variables
BPC_RIGHT_BRACKET
{
	$$ = NULL;
	// check identifier
	if (bpcsmtv_registery_identifier_test($sdd_msg_name)) {
		bpcyy_error_fmt("Duplicated identifier: \"%s\"", $sdd_msg_name);
		YYERROR;
	}

	// set data
	// setup field layout
	bpcsmtv_setup_field_layout($bpc_variables, $bpc_struct_modifier, $sdd_msg_name);
	// setup reliability
	bpcsmtv_setup_reliability($bpc_struct_modifier, $sdd_msg_name);
	// fill data
	$$ = bpcsmtv_constructor_msg();
	$$->msg_modifier = $bpc_struct_modifier;
	$$->msg_name = $sdd_msg_name;
	$$->msg_body = $bpc_variables;
	$$->msg_index = bpcsmtv_registery_identifier_distribute_index();
};

bpc_variable_array:
%empty
{
	$$ = bpcsmtv_constructor_variable_array();
	$$->is_array = false;
	$$->is_static_array = false;
	$$->static_array_len = 0u;
}
|
bpc_variable_array[sdd_parent_array] BPC_ARRAY_LIST
{
	$$ = NULL;
	// check duplicated assign
	if ($sdd_parent_array->is_array) {
		bpcyy_error_fmt("Duplicated array modifier.");
		YYERROR;
	}

	// set data
	$$ = $sdd_parent_array;
	$$->is_array = true;
	$$->is_static_array = false;
	$$->static_array_len = 0u;
}
|
bpc_variable_array[sdd_parent_array] BPC_ARRAY_TUPLE[sdd_tuple]
{
	$$ = NULL;
	// check duplicated assign
	if ($sdd_parent_array->is_array) {
		bpcyy_error_fmt("Duplicated array modifier.");
		YYERROR;
	}
	// check tuple size
	uint32_t parsed_size = UINT32_C(0);
	if (!bpcsmtv_get_offset_number(&($sdd_tuple), &parsed_size)) {
		bpcyy_error_fmt("Static array modifier is invalid. The reason has been written above.");
		YYERROR;
	}

	// set data
	$$ = $sdd_parent_array;
	$$->is_array = true;
	$$->is_static_array = true;
	$$->static_array_len = parsed_size;
};

bpc_variable_type:
BPC_TOKEN_NAME[sdd_custom_type]
{
	$$ = NULL;
	// validate identifier
	BPCSMTV_PROTOCOL_BODY* entry = bpcsmtv_registery_identifier_get($sdd_custom_type);
	if (entry == NULL) {
		bpcyy_error_fmt("Invalid variable data type \"%s\" . Please refer it after its defination.", $sdd_custom_type);
		YYERROR;
	} else if (entry->node_type == BPCSMTV_DEFINED_IDENTIFIER_TYPE_MSG) {
		bpcyy_error_fmt("Msg \"%s\" can not be set as data type. Consider using Struct please.", $sdd_custom_type);
		YYERROR;
	}

	// set data
	$$ = bpcsmtv_constructor_variable_type();
	$$->is_basic_type = false;
	$$->type_data.custom_type = $sdd_custom_type;

	// analyse
	bpcsmtv_analyse_underlaying_type($$);
}
|
BPC_BASIC_TYPE[sdd_basic_type]
{
	// set data
	$$ = bpcsmtv_constructor_variable_type();
	$$->is_basic_type = true;
	$$->type_data.basic_type = $sdd_basic_type;
	
	// analyse
	bpcsmtv_analyse_underlaying_type($$);
}

bpc_variable_align:
%empty
{
	$$ = bpcsmtv_constructor_variable_align();
	$$->use_align = false;
	$$->padding_size = 0u;
}
|
BPC_ALIGN[sdd_align]
{
	$$ = NULL;
	// check align size
	uint32_t parsed_align = UINT32_C(0);
	if (!bpcsmtv_get_offset_number(&($sdd_align), &parsed_align)) {
		bpcyy_error_fmt("Align modifier is invalid. The reason has been written above.");
		YYERROR;
	}

	// set data
	$$ = bpcsmtv_constructor_variable_align();
	$$->use_align = true;
	$$->padding_size = parsed_align;
};

bpc_variable_declarators:
BPC_TOKEN_NAME[sdd_name]
{
	$$ = NULL;
	// check variable duplication first
	if (bpcsmtv_registery_variables_test($sdd_name)) {
		bpcyy_error_fmt("duplicated variable: \"%s\"", $sdd_name);
		YYERROR;
	}

	// set data
	$$ = g_slist_append(NULL, $sdd_name);
	bpcsmtv_registery_variables_add($sdd_name);
}
|
bpc_variable_declarators[sdd_parent_declarators] BPC_TOKEN_NAME[sdd_name]
{
	$$ = NULL;
	// check variable duplication first
	if (bpcsmtv_registery_variables_test($sdd_name)) {
		bpcyy_error_fmt("Duplicated variable \"%s\"", $sdd_name);
		YYERROR;
	}

	// set data
	$$ = g_slist_append($sdd_parent_declarators, $sdd_name);
	bpcsmtv_registery_variables_add($sdd_name);
};

bpc_variables:
%empty
{
	$$ = NULL;
}
|
bpc_variables[sdd_parent_variables] bpc_variable_type bpc_variable_array bpc_variable_declarators bpc_variable_align BPC_SEMICOLON
{
	$$ = $sdd_parent_variables;
	// create variable for each declarator
	GSList* cursor;
	char* ptr;
	for (cursor = $bpc_variable_declarators; cursor != NULL; cursor = cursor->next) {
		ptr = (char*)cursor->data;

		// copy 3 modifier and name
		BPCSMTV_VARIABLE* data = bpcsmtv_constructor_variable();
		data->variable_type = bpcsmtv_duplicator_variable_type($bpc_variable_type);
		data->variable_array = bpcsmtv_duplicator_variable_array($bpc_variable_array);
		data->variable_align = bpcsmtv_duplicator_variable_align($bpc_variable_align);
		data->variable_name = g_strdup(ptr);

		// push data
		$$ = g_slist_append($$, data);
	}

	// free data
	bpcsmtv_destructor_variable_array($bpc_variable_array);
	bpcsmtv_destructor_variable_type($bpc_variable_type);
	bpcsmtv_destructor_variable_align($bpc_variable_align);
	bpcsmtv_destructor_slist_string($bpc_variable_declarators);
}
|
bpc_variables[sdd_parent_variables] error BPC_SEMICOLON
{
	$$ = $sdd_parent_variables;
	yyerrok;	// recover after detecting a BPC_SEMICOLON
};


%%

int yywrap() {
	return 1;
}

void yyerror(const char *s) {
	bpcerr_error(BPCERR_ERROR_SOURCE_PARSER, "%s\n\
	from ln:%d col:%d, to ln:%d col:%d", 
		s,
		yylloc.first_line, yylloc.first_column, yylloc.last_line, yylloc.last_column
	);
}

void bpcyy_error_fmt(const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	gchar* buf = bpcfs_vsprintf(format, ap);
	va_end(ap);

	// call real yyerror
	yyerror(buf);

	// free
	g_free(buf);
}

int bpcyy_run_compiler(BPCCMD_PARSED_ARGS* bpc_args, bpcyy_compiler_test_probe probe) {
	// setup test probe
	test_probe = probe;

	// setup stream
	yyout = stdout;
	yyin = bpc_args->input_file;
	if (yyin == NULL) {
		bpcerr_error(BPCERR_ERROR_SOURCE_PARSER, "Fail to open bp file stream. Compile aborted.");
		return 1;
	}

	// init code file
	bpcgen_init_code_file(bpc_args);

	// do parse
	bpcsmtv_registery_identifier_reset();
	bpcsmtv_registery_variables_reset();
	int result = yyparse();

	// free resources
	yyin = stdin;
	bpcgen_free_code_file();

	// free test probe
	test_probe = NULL;

	return result;
}
