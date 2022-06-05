%code top {

}

%code requires {
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "bpc_semantic_values.h"
}

%{
#include <stdio.h>
#include <string.h>

//int yydebug = 1;
extern FILE * yyin;
extern FILE * yyout;

extern int yylex(void);
extern int yyparse(void); 

int yywrap() {
	return 1;
}

void yyerror(const char *s)
{
	printf("[ERROR] %s\n", s);
}
%}

%union {
	bpc_basic_type_t token_basic_type;
	char* token_name;
	uint32_t token_num;
}
%token <token_num> BPC_TOKEN_NUM
%token <token_name> BPC_TOKEN_NAME
%token <token_basic_type> BPC_BASIC_TYPE
%token BPC_VERSION BPC_NAMESPACE BPC_LANGUAGE
%token BPC_ALIAS BPC_ENUM BPC_STRUCT BPC_MSG
%token BPC_ARRAY_TUPLE BPC_ARRAY_LIST
%token BPC_COLON BPC_COMMA BPC_SEMICOLON BPC_LEFT_BRACKET BPC_RIGHT_BRACKET BPC_DOT

%%

bpc_document:
BPC_VERSION BPC_TOKEN_NUM BPC_SEMICOLON 
BPC_NAMESPACE bpc_namespace_chain BPC_SEMICOLON 
BPC_LANGUAGE BPC_TOKEN_NAME BPC_SEMICOLON 
bpc_alias_group
bpc_define_group
{

};

bpc_namespace_chain:
BPC_TOKEN_NAME
{

}
|
bpc_namespace_chain BPC_DOT BPC_TOKEN_NAME
{

};

bpc_alias_group:
|
bpc_alias_group BPC_ALIAS BPC_TOKEN_NAME BPC_BASIC_TYPE BPC_SEMICOLON
{

};

bpc_define_group:
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
bpc_enum_body BPC_COMMA BPC_TOKEN_NAME
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
BPC_BASIC_TYPE BPC_TOKEN_NAME
{

}
|
BPC_TOKEN_NAME BPC_TOKEN_NAME
{

}
|
bpc_member BPC_COMMA BPC_TOKEN_NAME
{

};

%%
