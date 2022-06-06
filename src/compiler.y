%code top {

}

%code requires {
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "bpc_semantic_values.h"
#include "bpc_code_gen.h"
}

%code provides {
int run_compiler(const char* srcfile, const char* destfile);
}

%{

int yywrap() {
	return 1;
}

void yyerror(const char *s)
{
	printf("[ERROR] %s\n", s);
}
%}

%union {
	BPC_BASIC_TYPE token_basic_type;
	char* token_name;
	uint32_t token_num;
	GList* nterm_namespace;	// item is char*
	GList* nterm_enum;		// item is char*
	GList* nterm_members;	// item is BPC_SEMANTIC_MEMBER*
}
%token <token_num> BPC_TOKEN_NUM
%token <token_name> BPC_TOKEN_NAME
%token <token_basic_type> BPC_BASIC_TYPE
%token BPC_VERSION BPC_NAMESPACE BPC_LANGUAGE
%token BPC_ALIAS BPC_ENUM BPC_STRUCT BPC_MSG
%token BPC_ARRAY_TUPLE BPC_ARRAY_LIST
%token BPC_COLON BPC_COMMA BPC_SEMICOLON BPC_LEFT_BRACKET BPC_RIGHT_BRACKET BPC_DOT

%nterm <nterm_namespace> bpc_namespace_chain
%nterm <nterm_enum> bpc_enum_body
%nterm <nterm_members> bpc_member_collection bpc_member_array bpc_member

%%

bpc_document:
bpc_version bpc_language bpc_namespace bpc_alias_group bpc_define_group
{

};

bpc_version:
BPC_VERSION BPC_TOKEN_NUM[version_num] BPC_SEMICOLON 
{
	if ($version_num != BPC_COMPILER_VERSION) {
		// todo: throw error: unsupported version
	}
};

bpc_language:
BPC_LANGUAGE BPC_TOKEN_NAME[lang_name] BPC_SEMICOLON 
{
	BPC_LANGUAGE lang = bpc_parse_language_string($lang_name);
	if (lang == -1) {
		// todo: throw error: unsupported language
	}

	init_language(lang);
};

bpc_namespace:
BPC_NAMESPACE bpc_namespace_chain BPC_SEMICOLON 
{
	init_namespace($bpc_namespace_chain);
};


bpc_namespace_chain:
BPC_TOKEN_NAME[ns_name]
{
	$$ = NULL;
	$$ = g_slist_append($$, $ns_name);
}
|
bpc_namespace_chain[old_chain] BPC_DOT BPC_TOKEN_NAME[new_ns_name]
{
	$$ = $old_chain;
	$$ = g_slist_append($$, $new_ns_name);
};

bpc_alias_group:
|
bpc_alias_group BPC_ALIAS BPC_TOKEN_NAME[user_type] BPC_BASIC_TYPE[predefined_type] BPC_SEMICOLON
{
	write_alias($user_type, $predefined_type);
	free($user_type);
	free($predefined_type);
};

bpc_define_group:
%empty
{

}
|
bpc_define_group bpc_enum
{

}
|
bpc_define_group bpc_struct
{

}
|
bpc_define_group bpc_msg
{

};

bpc_enum:
BPC_ENUM BPC_TOKEN_NAME BPC_COLON BPC_BASIC_TYPE BPC_LEFT_BRACKET
bpc_enum_body
BPC_RIGHT_BRACKET
{

};

bpc_enum_body:
BPC_TOKEN_NAME
{
	
}
|
bpc_enum_body BPC_COMMA BPC_TOKEN_NAME[enum_item_name]
{

};

bpc_struct:
BPC_STRUCT BPC_TOKEN_NAME BPC_LEFT_BRACKET
bpc_member_collection
BPC_RIGHT_BRACKET
{

};

bpc_msg:
BPC_MSG BPC_TOKEN_NAME BPC_LEFT_BRACKET
bpc_member_collection
BPC_RIGHT_BRACKET
{

};

bpc_member_collection:
%empty
{

}
|
bpc_member_collection bpc_member_array
{

};

bpc_member_array:
bpc_member BPC_SEMICOLON
{

}
|
bpc_member BPC_ARRAY_TUPLE BPC_TOKEN_NUM BPC_SEMICOLON
{

}
|
bpc_member BPC_ARRAY_LIST BPC_SEMICOLON
{

};

bpc_member:
BPC_BASIC_TYPE[member_basic_type] BPC_TOKEN_NAME[member_name]
{
	BPC_SEMANTIC_MEMBER* data = bpc_constructor_semantic_member();
	data->is_basic_type = true;
	data->v_basic_type = $member_basic_type;
	data->vname = $member_name;

	$$ = NULL;
	$$ = g_slist_append($$, data);
}
|
BPC_TOKEN_NAME[member_user_type] BPC_TOKEN_NAME[member_name]
{
	BPC_SEMANTIC_MEMBER* data = bpc_constructor_semantic_member();
	data->is_basic_type = false;
	data->v_user_type = $member_user_type;
	data->vname = $member_name;

	$$ = NULL;
	$$ = g_slist_append($$, data);
}
|
bpc_member[old_member] BPC_COMMA BPC_TOKEN_NAME[member_name]
{
	BPC_SEMANTIC_MEMBER* existed_define = (BPC_SEMANTIC_MEMBER*)$old_member;

	BPC_SEMANTIC_MEMBER* data = bpc_constructor_semantic_member();
	data->is_basic_type = $old_member;
	if (data->is_basic_type) {
		data->v_basic_type = existed_define->v_basic_type;
		data->vname = $member_name;
	} else {
		data->v_user_type = g_strdup(existed_define->v_user_type);
		data->vname = $member_name;
	}

	$$ = $old_member;
	$$ = g_slist_append($$, data);
};

%%

int run_compiler(const char* srcfile, const char* destfile) {

}
