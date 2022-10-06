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
%token BPC_COMMA			","
%token BPC_SEMICOLON		";"
%token BPC_DOT				"."
%token BPC_LEFT_BRACKET		"{"
%token BPC_RIGHT_BRACKET	"}"
%token BPC_EQUAL			"="



%destructor { bpc_destructor_string($$); } <token_identifier>

%%

bpc_document:
bpc_version bpc_namespace bpc_protocol_body
{

};

bpc_version:
BPC_VERSION BPC_TOKEN_NUM[sdd_version_num] BPC_SEMICOLON
{

};

bpc_namespace:
BPC_NAMESPACE bpc_namespace_chain BPC_SEMICOLON
{

};

bpc_namespace_chain:
BPC_TOKEN_NAME[sdd_name]
{

}
|
bpc_namespace_chain[sdd_parent_chain] BPC_DOT BPC_TOKEN_NAME[sdd_name]
{

};

bpc_protocol_body:
%empty
{

}
|
bpc_protocol_body[sdd_parent_protocol_body] bpc_alias
{

}
|
bpc_protocol_body[sdd_parent_protocol_body] bpc_enum
{

}
|
bpc_protocol_body[sdd_parent_protocol_body] bpc_struct
{

}
|
bpc_protocol_body[sdd_parent_protocol_body] bpc_msg
{

}
|
bpc_protocol_body[sdd_parent_protocol_body] error BPC_RIGHT_BRACKET
{

}
|
bpc_protocol_body[sdd_parent_protocol_body] error BPC_SEMICOLON
{

};

bpc_alias:
BPC_ALIAS BPC_TOKEN_NAME[sdd_user_type] BPC_BASIC_TYPE[sdd_basic_type] BPC_SEMICOLON
{

};

bpc_enum:
BPC_ENUM BPC_BASIC_TYPE[sdd_enum_type] BPC_TOKEN_NAME[sdd_enum_name] BPC_LEFT_BRACKET
bpc_enum_body
BPC_RIGHT_BRACKET
{

};

bpc_enum_body:
BPC_TOKEN_NAME[sdd_name]
{

}
|
BPC_TOKEN_NAME[sdd_name] BPC_EQUAL BPC_TOKEN_NUM[sdd_spec_num]
{

}
|
bpc_enum_body[sdd_parent_enum_body] BPC_COMMA BPC_TOKEN_NAME[sdd_name]
{

}
|
bpc_enum_body[sdd_parent_enum_body] BPC_COMMA BPC_TOKEN_NAME[sdd_name] BPC_EQUAL BPC_TOKEN_NUM[sdd_spec_num]
{

};

bpc_struct_modifier:
%empty
{

}
|
bpc_struct_modifier[sdd_parent_modifier] BPC_RELIABILITY[sdd_reliability]
{

}
|
bpc_struct_modifier[sdd_parent_modifier] BPC_FIELD_LAYOUT[sdd_field_layout]
{

};

bpc_struct:
bpc_struct_modifier BPC_STRUCT BPC_TOKEN_NAME[sdd_struct_name] BPC_LEFT_BRACKET
bpc_variables
BPC_RIGHT_BRACKET
{

};

bpc_msg:
bpc_struct_modifier BPC_MSG BPC_TOKEN_NAME[sdd_msg_name] BPC_LEFT_BRACKET
bpc_variables
BPC_RIGHT_BRACKET
{

};

bpc_variable_array:
%empty
{

}
|
bpc_variable_array[sdd_parent_array] BPC_ARRAY_LIST
{

}
|
bpc_variable_array[sdd_parent_array] BPC_ARRAY_TUPLE[sdd_tuple]
{

};

bpc_variable_type:
BPC_TOKEN_NAME[sdd_custom_type]
{

}
|
BPC_BASIC_TYPE[sdd_basic_type]
{

}

bpc_variable_align:
%empty
{

}
|
BPC_ALIGN
{

};

bpc_variable_declarators:
BPC_TOKEN_NAME[sdd_name]
{

}
|
bpc_variable_declarators[sdd_parent_declarators] BPC_TOKEN_NAME[sdd_name]
{

};

bpc_variables:
%empty
{

}
|
bpc_variables[sdd_parent_variables] bpc_variable_type bpc_variable_array bpc_variable_declarators bpc_variable_align BPC_SEMICOLON
{

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
