%{
#include <stdio.h>
#include "y.tab.h"
#include "bpc_semantic_values.h"
#include "bpc_error.h"
#include <stdbool.h>

// https://www.techtalk7.com/how-does-flex-support-bison-location-exactly/
#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line; \
    yylloc.first_column = yylloc.last_column; \
    for(int i = 0; yytext[i] != '\0'; i++) { \
        if(yytext[i] == '\n') { \
            yylloc.last_line++; \
            yylloc.last_column = 1; \
        } \
        else { \
            yylloc.last_column++; \
        } \
    }

%}

LEGAL_BLANK		[ \t]+
TOKEN_NUM		[1-9][0-9]*
TOKEN_NAME		[a-zA-Z][a-zA-Z0-9_]*

%option never-interactive
%option nounistd

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
	yylval.token_basic_type = bpcsmtv_parse_basic_type(yytext);
	return BPC_BASIC_TYPE;
}
"[]" {
	return BPC_ARRAY_LIST;
}
"["{LEGAL_BLANK}{TOKEN_NUM}{LEGAL_BLANK}"]" {
	yylval.token_num = bpcsmtv_parse_number(yytext, yyleng, 1u, 1u);
	return BPC_ARRAY_TUPLE;
}
"reliable"|"unreliable" {
	yylval.token_bool = bpcsmtv_parse_reliability(yytext);
	return BPC_RELIABILITY;
}
"narrow"|"natural" {
	yylval.token_bool = bpcsmtv_parse_field_layout(yytext);
	return BPC_FIELD_LAYOUT;
}
"#"{LEGAL_BLANK}{TOKEN_NUM} {
	yylval.token_num = bpcsmtv_parse_number(yytext, yyleng, 1u, 0u);
	return BPC_ALIGN;
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
"=" {
	return BPC_EQUAL;
}

{TOKEN_NAME} {
	yylval.token_identifier = g_strdup(yytext);
	return BPC_TOKEN_NAME;
}

{TOKEN_NUM} {
	yylval.token_num = bpcsmtv_parse_number(yytext, yyleng, 0u, 0u);
	return BPC_TOKEN_NUM;
}

[ \t\n]+ /* ignore whitespace */;

.|\n {
	/* eat up any unmatched character */
	// and report error
	bpcerr_error(BPCERR_ERROR_SOURCE_LEX, "invalid text. %s", yytext);
}

%%

