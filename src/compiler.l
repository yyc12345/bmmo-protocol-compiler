%{
#include <stdio.h>
#include <io.h>
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
	yylval.token_basic_type = bpc_parse_basic_type_string(yytext);
	return BPC_BASIC_TYPE;
}
"tuple" {
	return BPC_ARRAY_TUPLE;
}
"list" {
	return BPC_ARRAY_LIST;
}
"reliable"|"unreliable" {
	yylval.token_bool = !strcmp(yytext, "reliable");
	return BPC_RELIABLE;
}
"align" {
	return BPC_ALIGN;
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
"=" {
	return BPC_EQUAL;
}

{TOKEN_NAME} {
	yylval.token_name = g_strdup(yytext);
	return BPC_TOKEN_NAME;
}

{TOKEN_NUM} {
	yylval.token_num = (int)g_ascii_strtoll(yytext, NULL, 10);
	return BPC_TOKEN_NUM;
}

[ \t\n]+ /* ignore whitespace */;

.|\n {
	/* eat up any unmatched character */
	// and report error
	bpcerr_error(BPCERR_ERROR_SOURCE_LEX, "invalid text. %s", yytext);
}

%%

