%{
#include <stdio.h>
#include "y.tab.h"
#include "bpc_semantic_values.h"
%}

TOKEN_NUM		[1-9][0-9]*
TOKEN_NAME		[a-zA-Z][a-zA-Z0-9_]*

%x BLOCK_COMMENT
%x LINE_COMMENT

%%

"/*" {
	BEGIN(BLOCK_COMMENT);
}
<BLOCK_COMMENT>[^*\n]*			/* eat anything that's not a `*` */;
<BLOCK_COMMENT>"*"+[^*/\n]*		/* eat up `*`'s not followed by `/` */;
<BLOCK_COMMENT>\n				/* ++line */;
<BLOCK_COMMENT>"*/" {
	BEGIN(INITIAL);
}

"//" {
	BEGIN(LINE_COMMENT);
}
<LINE_COMMENT>[^\n]+				/* eat up all char except new-line */;
<LINE_COMMENT>"\n" {
	BEGIN(INITIAL);
}

"bpc" {
	return BPC_VERSION;
}
"namespace" {
	return BPC_NAMESPACE;
}
"language" {
	return BPC_LANGUAGE;
}
"alias" {
	return BPC_ALIAS;
}
"enum" {
	return BPC_ENUM;
}
"struct" {
	return BPC_STRUCT;
}
"msg" {
	return BPC_MSG;
}
"float"|"double"|"int8"|"int16"|"int32"|"int64"|"uint8"|"uint16"|"uint32"|"uint64"|"string" {
	yylval.token_basic_type = bpc_parse_basic_type_string(yytext);
	return BPC_BASIC_TYPE;
}
"tuple" {
	return BPC_ARRAY_TUPLE;
}
"list" {
	return BPC_ARRAY_LIST;
}
":" {
	return BPC_COLON;
}
"," {
	return BPC_COMMA;
}
";" {
	return BPC_SEMICOLON;
}
"." {
	return BPC_DOT;
}
"{" {
	return BPC_LEFT_BRACKET;
}
"}" {
	return BPC_RIGHT_BRACKET;
}

{TOKEN_NAME} {
	yylval.token_name = g_strdup(yytext);
	return BPC_TOKEN_NAME;
}

{TOKEN_NUM} {
	yylval.token_num = (int)atol(yytext, NULL, 10);
	return BPC_TOKEN_NUM;
}

[ \t\n]+ /* ignore whitespace */;

%%

