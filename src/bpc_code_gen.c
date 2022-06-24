#include "bpc_code_gen.h"
#include <stdio.h>

//const uint32_t bpc_codegen_basic_type_size[] = {
//	4, 8, 1, 2, 4, 8, 1, 2, 4, 8
//};

const char* bpc_codegen_python_struct_pattern[] = {
	"f",
	"d",
	"b",
	"h",
	"i",
	"q",
	"B",
	"H",
	"I",
	"Q"
};
const char* bpc_codegen_python_struct_pattern_len[] = {
	"4",
	"8",
	"1",
	"2",
	"4",
	"8",
	"1",
	"2",
	"4",
	"8"
};


FILE* codegen_fileptr = NULL;
BPC_SEMANTIC_LANGUAGE codegen_spec_lang = BPC_SEMANTIC_LANGUAGE_PYTHON;
GSList* codegen_user_defined_token_slist = NULL;
uint32_t codegen_msg_counter = 0;

#define BPC_CODEGEN_INDENT_INIT uint32_t _indent_level = 0, _indent_loop = 0;
#define BPC_CODEGEN_INDENT_INC ++_indent_level;
#define BPC_CODEGEN_INDENT_DEC --_indent_level;
#define BPC_CODEGEN_INDENT_PRINT fputc('\n', codegen_fileptr); for(_indent_loop=0;_indent_loop<_indent_level;++_indent_loop) fputc('\t', codegen_fileptr);

bool bpc_codegen_init_code_file(const char* filepath) {
	codegen_fileptr = fopen(filepath, "w+");
	if (codegen_fileptr == NULL) return false;

	codegen_user_defined_token_slist = NULL;
	codegen_msg_counter = 0;
	return true;
}

void bpc_codegen_init_language(BPC_SEMANTIC_LANGUAGE lang) {
	codegen_spec_lang = lang;
	switch (codegen_spec_lang) {
		case BPC_SEMANTIC_LANGUAGE_CPP:
			printf("[Warning] Cpp code gen is unsupported now. It will come soon.\n");
			break;
		case BPC_SEMANTIC_LANGUAGE_CSHARP:
			printf("[Warning] Csharp code gen is unsupported now. It will come soon.\n");
			break;
		case BPC_SEMANTIC_LANGUAGE_PYTHON:
			_bpc_codegen_copy_template("snippets/header.py");
			break;
		default:
			printf("[Warning] Unknow language type.\n");
			break;
	}
}

void bpc_codegen_init_namespace(GSList* namespace_chain) {
	switch (codegen_spec_lang) {
		case BPC_SEMANTIC_LANGUAGE_CPP:
		{


		}
		break;
		case BPC_SEMANTIC_LANGUAGE_CSHARP:
		{


		}
		break;
		case BPC_SEMANTIC_LANGUAGE_PYTHON:
		{
			;
			// do nothing, because python do not have namespace.
		}
		break;
	}
}

BPC_CODEGEN_TOKEN_ENTRY* bpc_codegen_get_token_entry(const char* token_name) {
	GSList* cursor;
	for (cursor = codegen_user_defined_token_slist; cursor != NULL; cursor = cursor->next) {
		BPC_CODEGEN_TOKEN_ENTRY* data = (BPC_CODEGEN_TOKEN_ENTRY*)cursor->data;
		if (g_str_equal(token_name, data->token_name)) return data;
	}
	return NULL;
}

void bpc_codegen_write_alias(const char* alias_name, BPC_SEMANTIC_BASIC_TYPE basic_t) {
	// alloc entry and push into list
	BPC_CODEGEN_TOKEN_ENTRY* entry = g_new0(BPC_CODEGEN_TOKEN_ENTRY, 1);
	entry->token_name = g_strdup(alias_name);
	entry->token_type = BPC_CODEGEN_TOKEN_TYPE_ALIAS;
	entry->token_basic_type = basic_t;
	codegen_user_defined_token_slist = g_slist_append(codegen_user_defined_token_slist, entry);

	switch (codegen_spec_lang) {
		case BPC_SEMANTIC_LANGUAGE_CPP:
		{


		}
		break;
		case BPC_SEMANTIC_LANGUAGE_CSHARP:
		{


		}
		break;
		case BPC_SEMANTIC_LANGUAGE_PYTHON:
		{
			;
			// do nothing, because python is weak-type lang.
			// each member do not declare its type expilct.
			// so alias is useless
		}
		break;
	}
}

void bpc_codegen_write_enum(const char* enum_name, BPC_SEMANTIC_BASIC_TYPE basic_type, GSList* member_list) {
	// alloc entry and push into list
	BPC_CODEGEN_TOKEN_ENTRY* entry = g_new0(BPC_CODEGEN_TOKEN_ENTRY, 1);
	entry->token_name = g_strdup(enum_name);
	entry->token_type = BPC_CODEGEN_TOKEN_TYPE_ENUM;
	entry->token_basic_type = basic_type;
	codegen_user_defined_token_slist = g_slist_append(codegen_user_defined_token_slist, entry);

	// define some useful variable
	BPC_CODEGEN_INDENT_INIT;
	GSList* cursor;
	uint32_t counter = 0;

	switch (codegen_spec_lang) {
		case BPC_SEMANTIC_LANGUAGE_CPP:
		{


		}
		break;
		case BPC_SEMANTIC_LANGUAGE_CSHARP:
		{


		}
		break;
		case BPC_SEMANTIC_LANGUAGE_PYTHON:
		{
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(codegen_fileptr, "class %s():", enum_name); BPC_CODEGEN_INDENT_INC;
			for (cursor = member_list; cursor != NULL; cursor = cursor->next) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(codegen_fileptr, "%s = %d", (char*)cursor->data, counter++);
			}
			// enum is over
			BPC_CODEGEN_INDENT_DEC;

		}
		break;
	}
}

void bpc_codegen_write_struct(const char* struct_name, GSList* member_list) {
	_bpc_codegen_gen_struct_msg_body(struct_name, member_list, NULL);
}
void bpc_codegen_write_msg(const char* msg_name, GSList* member_list, bool is_reliable) {
	BPC_CODEGEN_MSG_EXTRA_PROPS props;
	props.is_reliable = is_reliable;
	_bpc_codegen_gen_struct_msg_body(msg_name, member_list, &props);
}
void _bpc_codegen_gen_struct_msg_body(const char* token_name, GSList* member_list, BPC_CODEGEN_MSG_EXTRA_PROPS* msg_prop) {
	bool is_msg = msg_prop != NULL;

	// generate msg index
	uint32_t msg_index = 0;
	if (is_msg) msg_index = codegen_msg_counter++;

	// alloc entry and push into list
	BPC_CODEGEN_TOKEN_ENTRY* entry = g_new0(BPC_CODEGEN_TOKEN_ENTRY, 1);
	entry->token_name = g_strdup(token_name);
	if (is_msg) {
		entry->token_type = BPC_CODEGEN_TOKEN_TYPE_MSG;
		entry->token_arranged_index = msg_index;
	} else {
		entry->token_type = BPC_CODEGEN_TOKEN_TYPE_STRUCT;
	}
	codegen_user_defined_token_slist = g_slist_append(codegen_user_defined_token_slist, entry);

	// define some useful variable
	BPC_CODEGEN_INDENT_INIT;
	GSList* cursor;
	GString* operator_name = g_string_new(NULL);
	bool proc_like_basic_type;
	BPC_SEMANTIC_BASIC_TYPE underlaying_basic_type;

	switch (codegen_spec_lang) {
		case BPC_SEMANTIC_LANGUAGE_CPP:
		{


		}
		break;
		case BPC_SEMANTIC_LANGUAGE_CSHARP:
		{


		}
		break;
		case BPC_SEMANTIC_LANGUAGE_PYTHON:
		{
			// generate header and props
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(codegen_fileptr, "class %s():", token_name); BPC_CODEGEN_INDENT_INC;
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(codegen_fileptr, "def __init__(self):");

			BPC_CODEGEN_INDENT_INC;
			for (cursor = member_list; cursor != NULL; cursor = cursor->next) {
				BPC_SEMANTIC_MEMBER* data = (BPC_SEMANTIC_MEMBER*)cursor->data;

				BPC_CODEGEN_INDENT_PRINT;
				if (data->array_prop.is_array) {
					fprintf(codegen_fileptr, "self.%s = []", data->vname);
				} else {
					fprintf(codegen_fileptr, "self.%s = None", data->vname);
				}
			}
			if (member_list == NULL) {
				// if there are no member, wee need write extra `pass`
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(codegen_fileptr, "pass");
			}
			BPC_CODEGEN_INDENT_DEC;

			// generate reliable getter and opcode getter for msg
			if (is_msg) {
				// reliable
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(codegen_fileptr, "def get_reliable(self) -> bool:"); BPC_CODEGEN_INDENT_INC;

				BPC_CODEGEN_INDENT_PRINT;
				if (msg_prop->is_reliable) fprintf(codegen_fileptr, "return True");
				else fprintf(codegen_fileptr, "return False");
				BPC_CODEGEN_INDENT_DEC;

				// opcode
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(codegen_fileptr, "def get_opcode(self) -> int:"); 
				
				BPC_CODEGEN_INDENT_INC;
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(codegen_fileptr, "return %d", msg_index);
				BPC_CODEGEN_INDENT_DEC;
			}

			// generate deserialize func
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(codegen_fileptr, "def deserialize(self, ss: io.BytesIO):"); BPC_CODEGEN_INDENT_INC;
			if (is_msg) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(codegen_fileptr, "if struct.unpack('I', ss.read(4))[0] != %d:", msg_index); BPC_CODEGEN_INDENT_INC;
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(codegen_fileptr, "raise Exception('invalid opcode!')"); BPC_CODEGEN_INDENT_DEC;
			}
			for (cursor = member_list; cursor != NULL; cursor = cursor->next) {
				BPC_SEMANTIC_MEMBER* data = (BPC_SEMANTIC_MEMBER*)cursor->data;

				// annotation
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(codegen_fileptr, "# %s", data->vname);

				// if member is array, we need construct a loop
				if (data->array_prop.is_array) {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(codegen_fileptr, "self.%s.clear()", data->vname);
					if (data->array_prop.is_static_array) {
						BPC_CODEGEN_INDENT_PRINT;
						fprintf(codegen_fileptr, "for _i in range(%d):", data->array_prop.array_len);
					} else {
						BPC_CODEGEN_INDENT_PRINT;
						fprintf(codegen_fileptr, "_count = struct.unpack('I', ss.read(4))[0]");
						BPC_CODEGEN_INDENT_PRINT;
						fprintf(codegen_fileptr, "for _i in range(_count):");
					}

					// increase indent level for array item
					BPC_CODEGEN_INDENT_INC;
				}

				// determine operator name
				if (data->array_prop.is_array) {
					g_string_printf(operator_name, "_cache");
				} else {
					g_string_printf(operator_name, "self.%s", data->vname);
				}

				// resolve basic type
				_bpc_codegen_get_underlaying_type(data, &proc_like_basic_type, &underlaying_basic_type);
				// determine read method
				if (proc_like_basic_type) {
					// basic type deserialize
					if (underlaying_basic_type == BPC_SEMANTIC_BASIC_TYPE_STRING) {
						// string is special
						BPC_CODEGEN_INDENT_PRINT;
						fprintf(codegen_fileptr, "_strlen = struct.unpack('I', ss.read(4))[0]");
						BPC_CODEGEN_INDENT_PRINT;
						fprintf(codegen_fileptr, "%s = ss.read(_strlen).decode(encoding='gb2312', errors='ignore')", operator_name->str);
					} else {
						// other value type can use table to generate
						BPC_CODEGEN_INDENT_PRINT;
						fprintf(codegen_fileptr, "%s = struct.unpack('%s', ss.read(%s))[0]",
							operator_name->str,
							bpc_codegen_python_struct_pattern[(size_t)underlaying_basic_type],
							bpc_codegen_python_struct_pattern_len[(size_t)underlaying_basic_type]);
					}
				} else {
					// struct deserialize
					// call struct.deserialize()
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(codegen_fileptr, "%s = %s()", operator_name->str, data->v_struct_type);
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(codegen_fileptr, "%s.deserialize(ss)", operator_name->str);
				}
				// compute align data
				if (data->align_prop.use_align) {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(codegen_fileptr, "ss.read(%d)", data->align_prop.padding_size);
				}

				// array extra proc
				if (data->array_prop.is_array) {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(codegen_fileptr, "self.%s.append(_cache)", data->vname);

					// shink indent
					BPC_CODEGEN_INDENT_DEC;
				}
			}
			if (member_list == NULL && (!is_msg)) {
				// if there are no member, and this struct is not msg,
				// we need write extra `pass`
				// because msg have at least one entry `opcode` so it doesn't need pass
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(codegen_fileptr, "pass");
			}
			// func deserialize is over
			BPC_CODEGEN_INDENT_DEC;

			// generate serialize func
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(codegen_fileptr, "def serialize(self, ss: io.BytesIO):"); BPC_CODEGEN_INDENT_INC;
			if (is_msg) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(codegen_fileptr, "ss.write(struct.pack('I', %d))", msg_index);
			}
			for (cursor = member_list; cursor != NULL; cursor = cursor->next) {
				BPC_SEMANTIC_MEMBER* data = (BPC_SEMANTIC_MEMBER*)cursor->data;

				// annotation
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(codegen_fileptr, "# %s", data->vname);

				// if member is array, we need construct a loop
				if (data->array_prop.is_array) {
					// increase indent level for array
					if (data->array_prop.is_static_array) {
						BPC_CODEGEN_INDENT_PRINT;
						fprintf(codegen_fileptr, "for _i in range(%d):", data->array_prop.array_len); BPC_CODEGEN_INDENT_INC;
						BPC_CODEGEN_INDENT_PRINT;
						fprintf(codegen_fileptr, "_item = self.%s[_i]", data->vname);
					} else {
						BPC_CODEGEN_INDENT_PRINT;
						fprintf(codegen_fileptr, "ss.write('I', len(self.%s))", data->vname);
						BPC_CODEGEN_INDENT_PRINT;
						fprintf(codegen_fileptr, "for _item in self.%s:", data->vname); BPC_CODEGEN_INDENT_INC;
					}
				}

				// determine operator name
				if (data->array_prop.is_array) {
					g_string_printf(operator_name, "_item");
				} else {
					g_string_printf(operator_name, "self.%s", data->vname);
				}

				// determine read method
				_bpc_codegen_get_underlaying_type(data, &proc_like_basic_type, &underlaying_basic_type);
				if (proc_like_basic_type) {
					// basic type serialize
					if (underlaying_basic_type == BPC_SEMANTIC_BASIC_TYPE_STRING) {
						// string is special in basic type
						BPC_CODEGEN_INDENT_PRINT;
						fprintf(codegen_fileptr, "_binstr = %s.encode(encoding='gb2312', errors='ignore')", operator_name->str);
						BPC_CODEGEN_INDENT_PRINT;
						fprintf(codegen_fileptr, "ss.write(struct.pack('I', len(_binstr)))");
						BPC_CODEGEN_INDENT_PRINT;
						fprintf(codegen_fileptr, "ss.write(_binstr)");
					} else {
						// other value can be generated by table search
						BPC_CODEGEN_INDENT_PRINT;
						fprintf(codegen_fileptr, "ss.write(struct.pack('%s', %s))",
							bpc_codegen_python_struct_pattern[(size_t)underlaying_basic_type],
							operator_name->str);
					}
				} else {
					// struct serialize
					// call struct.serialize()
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(codegen_fileptr, "%s.serialize(ss)", operator_name->str);
				}
				// compute align size
				if (data->align_prop.use_align) {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(codegen_fileptr, "ss.write(b'\\0' * %d)", data->align_prop.padding_size);
				}

				// array extra proc
				if (data->array_prop.is_array) {
					// shink indent
					BPC_CODEGEN_INDENT_DEC;
				}
			}
			if (member_list == NULL && (!is_msg)) {
				// if there are no member, and this struct is not msg,
				// we need write extra `pass`
				// because msg have at least one entry `opcode` so it doesn't need pass
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(codegen_fileptr, "pass");
			}
			// func deserialize is over
			BPC_CODEGEN_INDENT_DEC;

			// class define is over
			BPC_CODEGEN_INDENT_DEC;

		}
		break;
	}

	g_string_free(operator_name, TRUE);
}

void bpc_codegen_write_opcode() {
	// define useful variable
	BPC_CODEGEN_INDENT_INIT;
	GSList* cursor;

	switch (codegen_spec_lang) {
		case BPC_SEMANTIC_LANGUAGE_CPP:
		{


		}
		break;
		case BPC_SEMANTIC_LANGUAGE_CSHARP:
		{


		}
		break;
		case BPC_SEMANTIC_LANGUAGE_PYTHON:
		{
			// write opcode enum
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(codegen_fileptr, "class opcode():"); BPC_CODEGEN_INDENT_INC;
			for (cursor = codegen_user_defined_token_slist; cursor != NULL; cursor = cursor->next) {
				BPC_CODEGEN_TOKEN_ENTRY* data = (BPC_CODEGEN_TOKEN_ENTRY*)cursor->data;

				if (data->token_type == BPC_CODEGEN_TOKEN_TYPE_MSG) {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(codegen_fileptr, "%s = %d", data->token_name, data->token_arranged_index);
				}
			}
			// class opcode is over
			BPC_CODEGEN_INDENT_DEC;

			// write uniformed deserialize func
			bool is_first = true;
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(codegen_fileptr, "def uniform_deserialize(ss: io.BytesIO):"); BPC_CODEGEN_INDENT_INC;
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(codegen_fileptr, "_opcode = _peek_opcode(ss)");
			for (cursor = codegen_user_defined_token_slist; cursor != NULL; cursor = cursor->next) {
				BPC_CODEGEN_TOKEN_ENTRY* data = (BPC_CODEGEN_TOKEN_ENTRY*)cursor->data;

				if (data->token_type == BPC_CODEGEN_TOKEN_TYPE_MSG) {
					BPC_CODEGEN_INDENT_PRINT;
					if (is_first) {
						fprintf(codegen_fileptr, "if _opcode == opcode.%s:", data->token_name);
						is_first = false;
					} else {
						fprintf(codegen_fileptr, "elif _opcode == opcode.%s:", data->token_name);
					}

					// write if body
					BPC_CODEGEN_INDENT_INC;
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(codegen_fileptr, "_data = %s()", data->token_name);
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(codegen_fileptr, "_data.deserialize(ss)");
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(codegen_fileptr, "return _data");
					BPC_CODEGEN_INDENT_DEC;
				}
			}
			// default return
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(codegen_fileptr, "return None");
			// uniform func is over
			BPC_CODEGEN_INDENT_DEC;

		}
		break;
	}
}

void bpc_codegen_free_code_file() {
	if (codegen_fileptr) {
		fclose(codegen_fileptr);
	}
	if (codegen_user_defined_token_slist) {
		g_slist_free_full(codegen_user_defined_token_slist, _bpc_codegen_free_token_entry);
	}
}

//uint32_t _bpc_codegen_get_align_padding_size(BPC_SEMANTIC_MEMBER* token, BPC_SEMANTIC_BASIC_TYPE underlaying_type) {
//	if (!token->align_prop.use_align) return 0;
//	int32_t original_size = bpc_codegen_basic_type_size[underlaying_type];
//	int32_t padding_size = token->align_prop.expected_size - original_size;
//	if (padding_size > 0) return (uint32_t)padding_size;
//	else return 0u;
//}

void _bpc_codegen_get_underlaying_type(BPC_SEMANTIC_MEMBER* token, bool* pout_proc_like_basic_type, BPC_SEMANTIC_BASIC_TYPE* pout_underlaying_basic_type) {
	if (token->is_basic_type) {
		*pout_proc_like_basic_type = true;
		*pout_underlaying_basic_type = token->v_basic_type;
	} else {
		// check user defined type
		// token type will never be msg accoring to syntax define
		BPC_CODEGEN_TOKEN_ENTRY* entry = bpc_codegen_get_token_entry(token->v_struct_type);
		if (entry->token_type == BPC_CODEGEN_TOKEN_TYPE_STRUCT) {
			// struct
			*pout_proc_like_basic_type = false;
		} else {
			// alias and enum
			*pout_proc_like_basic_type = true;
			*pout_underlaying_basic_type = entry->token_basic_type;
		}
	}
}

void _bpc_codegen_copy_template(const char* template_code_file_path) {
	FILE* template_fs = fopen(template_code_file_path, "r+");
	if (template_fs == NULL) {
		printf("[Warning] Fail to open code template file: %s\n", template_code_file_path);
		printf("[Warning] Parse will continue, but generated code may not work.\n");
		return;
	}

	char* buffer = g_new0(char, 1024);
	size_t read_counter;

	while ((read_counter = fread(buffer, sizeof(char), 1024, template_fs)) != 0) {
		fwrite(buffer, sizeof(char), read_counter, codegen_fileptr);
	}

	fclose(template_fs);
}

void _bpc_codegen_free_token_entry(gpointer rawptr) {
	if (rawptr == NULL) return;

	BPC_CODEGEN_TOKEN_ENTRY* ptr = (BPC_CODEGEN_TOKEN_ENTRY*)rawptr;
	if (ptr->token_name != NULL) g_free(ptr->token_name);
	g_free(ptr);
}
