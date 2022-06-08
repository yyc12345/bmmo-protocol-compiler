#include "bpc_code_gen.h"
#include <stdio.h>

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
const size_t bpc_codegen_python_struct_pattern_len = sizeof(bpc_codegen_python_struct_pattern) / sizeof(char*);

FILE* codegen_fileptr = NULL;
BPC_SEMANTIC_LANGUAGE codegen_spec_lang = BPC_SEMANTIC_LANGUAGE_PYTHON;
GSList* codegen_user_defined_token_slist = NULL;
uint32_t codegen_msg_counter = 0;

bool bpc_codegen_init_code_file(const char* filepath) {
	codegen_fileptr = fopen(filepath, "w+");
	if (codegen_fileptr == NULL) return false;

	codegen_user_defined_token_slist = NULL;
	codegen_msg_counter = 0;
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

void bpc_codegen_init_namespace(GList* namespace_chain) {
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
			GSList* cursor;
			uint32_t counter = 0;

			fprintf(codegen_fileptr, "class %s():\n", enum_name);
			for (cursor = member_list; cursor != NULL; cursor = cursor->next) {
				char* data = (char*)cursor->data;
				fprintf("\t%s = %d\n", data, counter++);
				cursor = cursor->next;
			}
		}
		break;
	}
}

void bpc_codegen_write_struct(const char* struct_name, GSList* member_list) {
	_bpc_codegen_gen_struct_msg_body(struct_name, member_list, false);
}
void bpc_codegen_write_msg(const char* msg_name, GSList* member_list) {
	_bpc_codegen_gen_struct_msg_body(msg_name, member_list, true);
}
void _bpc_codegen_gen_struct_msg_body(const char* token_name, GSList* member_list, bool is_msg) {
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
			GSList* cursor;
			GString* array_item_head = g_string_new(NULL);

			// generate header and props
			fprintf(codegen_fileptr, "class %s():\n", token_name);
			fprintf(codegen_fileptr, "\tdef __init__(self):\n");

			for (cursor = member_list; cursor != NULL; cursor = cursor->next) {
				BPC_SEMANTIC_MEMBER* data = (BPC_SEMANTIC_MEMBER*)cursor->data;
				if (data->array_prop.is_array) {
					fprintf(codegen_fileptr, "\t\tself.%s = []\n", data->vname);
				} else {
					fprintf(codegen_fileptr, "\t\tself.%s = None\n", data->vname);
				}
				cursor = cursor->next;
			}

			// generate deserialize func
			fprintf(codegen_fileptr, "\tdef deserialize(ss: io.BytesIO)\n");
			if (is_msg) {
				fprintf(codegen_fileptr, "\t\tif struct.unpack('I', ss.read(4))[0] != %d:\n", msg_index);
				fprintf(codegen_fileptr, "\t\t\traise Exception('invalid opcode!')\n");
			}
			for (cursor = member_list; cursor != NULL; cursor = cursor->next) {
				BPC_SEMANTIC_MEMBER* data = (BPC_SEMANTIC_MEMBER*)cursor->data;

				// if member is array, we need construct a loop
				if (data->array_prop.is_array) {
					fprintf(codegen_fileptr, "\t\tself.%s.clear()\n", data->vname);
					if (data->array_prop.is_static_array) {
						fprintf(codegen_fileptr, "\t\tfor _i in range(%d):\n", data->array_prop.array_len);
					} else {
						fprintf(codegen_fileptr, "\t\t_count = struct.unpack('I', ss.read(4))[0]\n");
						fprintf(codegen_fileptr, "\t\tfor _i in range(_count):\n");
					}
				}

				// determine reader level
				if (data->array_prop.is_array) {
					g_string_printf(array_item_head, "\t\t\t_cache");
				} else {
					g_string_printf(array_item_head, "\t\tself.%s", data->vname);
				}

				// determine read method
				bool is_real_basic_type;
				BPC_SEMANTIC_BASIC_TYPE real_basic_type;
				if (data->is_basic_type) {
					is_real_basic_type = true;
					real_basic_type = data->v_basic_type;
				} else {
					BPC_CODEGEN_TOKEN_ENTRY* entry = bpc_codegen_get_token_entry(data->v_struct_type);
					if (entry->token_type == BPC_CODEGEN_TOKEN_TYPE_STRUCT) {
						is_real_basic_type = false;
					} else {
						is_real_basic_type = true;
						real_basic_type = entry->token_basic_type;
					}
				}
				if (is_real_basic_type) {
					// basic type serialize
					switch (real_basic_type) {
						case BPC_SEMANTIC_BASIC_TYPE_FLOAT:
							fprintf(codegen_fileptr, "%s = struct.unpack('f', ss.read(4))[0]\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_DOUBLE:
							fprintf(codegen_fileptr, "%s = struct.unpack('d', ss.read(8))[0]\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_INT8:
							fprintf(codegen_fileptr, "%s = struct.unpack('b', ss.read(1))[0]\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_INT16:
							fprintf(codegen_fileptr, "%s = struct.unpack('h', ss.read(2))[0]\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_INT32:
							fprintf(codegen_fileptr, "%s = struct.unpack('i', ss.read(4))[0]\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_INT64:
							fprintf(codegen_fileptr, "%s = struct.unpack('q', ss.read(8))[0]\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_UINT8:
							fprintf(codegen_fileptr, "%s = struct.unpack('B', ss.read(1))[0]\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_UINT16:
							fprintf(codegen_fileptr, "%s = struct.unpack('H', ss.read(2))[0]\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_UINT32:
							fprintf(codegen_fileptr, "%s = struct.unpack('I', ss.read(4))[0]\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_UINT64:
							fprintf(codegen_fileptr, "%s = struct.unpack('Q', ss.read(8))[0]\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_STRING:
							fprintf(codegen_fileptr, "%s = struct.unpack('f', ss.read(4))[0]\n", array_item_head->str);
							fprintf(codegen_fileptr, "%s = ss.read(_count).decode(encoding='gb2312', errors='ignore')\n", array_item_head->str);
							break;
					}
				} else {
					// call struct.deserialize()
					fprintf(codegen_fileptr, "%s = %s()\n", array_item_head->str, data->v_struct_type);
					fprintf(codegen_fileptr, "%s.deserialize(ss)\n", array_item_head->str);
				}

				if (data->array_prop.is_array) {
					fprintf(codegen_fileptr, "\t\t\tself.%s.append(_cache)\n", data->vname);
				}
			}

			// generate serialize func
			fprintf(codegen_fileptr, "\tdef serialize(ss: io.BytesIO)\n");
			if (is_msg) {
				fprintf(codegen_fileptr, "\t\tss.write(struct.pack('I', %d))\n", msg_index);
			}
			for (cursor = member_list; cursor != NULL; cursor = cursor->next) {
				BPC_SEMANTIC_MEMBER* data = (BPC_SEMANTIC_MEMBER*)cursor->data;

				// if member is array, we need construct a loop
				if (data->array_prop.is_array) {
					if (data->array_prop.is_static_array) {
						fprintf(codegen_fileptr, "\t\tfor _i in range(%d):\n", data->array_prop.array_len);
						fprintf(codegen_fileptr, "\t\t\t_item = self.%s[_i]\n", data->vname);
					} else {
						fprintf(codegen_fileptr, "\t\tss.write('I', len(self.%s))\n", data->vname);
						fprintf(codegen_fileptr, "\t\tfor _item in self.%s:\n", data->vname);
					}
				}

				// determine reader level
				if (data->array_prop.is_array) {
					g_string_printf(array_item_head, "\t\t\t");
				} else {
					g_string_printf(array_item_head, "\t\t");
				}

				// determine read method
				bool is_real_basic_type;
				BPC_SEMANTIC_BASIC_TYPE real_basic_type;
				if (data->is_basic_type) {
					is_real_basic_type = true;
					real_basic_type = data->v_basic_type;
				} else {
					BPC_CODEGEN_TOKEN_ENTRY* entry = bpc_codegen_get_token_entry(data->v_struct_type);
					if (entry->token_type == BPC_CODEGEN_TOKEN_TYPE_STRUCT) {
						is_real_basic_type = false;
					} else {
						is_real_basic_type = true;
						real_basic_type = entry->token_basic_type;
					}
				}
				if (is_real_basic_type) {
					// basic type serialize
					switch (real_basic_type) {
						case BPC_SEMANTIC_BASIC_TYPE_FLOAT:
							fprintf(codegen_fileptr, "%sss.write(struct.unpack('f', item))\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_DOUBLE:
							fprintf(codegen_fileptr, "%sss.write(struct.unpack('d', item))\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_INT8:
							fprintf(codegen_fileptr, "%sss.write(struct.unpack('b', item))\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_INT16:
							fprintf(codegen_fileptr, "%sss.write(struct.unpack('h', item))\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_INT32:
							fprintf(codegen_fileptr, "%sss.write(struct.unpack('i', item))\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_INT64:
							fprintf(codegen_fileptr, "%sss.write(struct.unpack('q', item))\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_UINT8:
							fprintf(codegen_fileptr, "%sss.write(struct.unpack('B', item))\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_UINT16:
							fprintf(codegen_fileptr, "%sss.write(struct.unpack('H', item))\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_UINT32:
							fprintf(codegen_fileptr, "%sss.write(struct.unpack('I', item))\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_UINT64:
							fprintf(codegen_fileptr, "%sss.write(struct.unpack('Q', item))\n", array_item_head->str);
							break;
						case BPC_SEMANTIC_BASIC_TYPE_STRING:
							fprintf(codegen_fileptr, "%s_binstr = item.encode(encoding='gb2312', errors='ignore')\n", array_item_head->str);
							fprintf(codegen_fileptr, "%sss.write(struct.unpack('I', len(_binstr)))\n", array_item_head->str);
							fprintf(codegen_fileptr, "%sss.write(_binstr)\n", array_item_head->str);
							break;
					}
				} else {
					// call struct.serialize()
					fprintf(codegen_fileptr, "%sitem.serialize(ss)\n", array_item_head->str);
				}

			}

			g_string_free(array_item_head, TRUE);
		}
		break;
	}
}

void bpc_codegen_write_opcode() {
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
			GSList* cursor;

			// write opcode enum
			fprintf(codegen_fileptr, "class opcode():\n");
			for (cursor = codegen_user_defined_token_slist; cursor != NULL; cursor = cursor->next) {
				BPC_CODEGEN_TOKEN_ENTRY* data = (BPC_CODEGEN_TOKEN_ENTRY*)cursor->data;

				if (data->token_type == BPC_CODEGEN_TOKEN_TYPE_MSG) {
					fprintf(codegen_fileptr, "\t%s = %d\n", data->token_name, data->token_arranged_index);
				}
			}

			// write uniformed deserialize func
			bool is_first = true;
			fprintf(codegen_fileptr, "def uniform_deserialize(ss: io.BytesIO):\n");
			fprintf(codegen_fileptr, "\t_opcode = _peek_opcode(ss)\n");
			for (cursor = codegen_user_defined_token_slist; cursor != NULL; cursor = cursor->next) {
				BPC_CODEGEN_TOKEN_ENTRY* data = (BPC_CODEGEN_TOKEN_ENTRY*)cursor->data;

				if (data->token_type == BPC_CODEGEN_TOKEN_TYPE_MSG) {
					if (is_first) {
						fprintf(codegen_fileptr, "if _opcode == opcode.%s:\n", data->token_name);
						is_first = false;
					} else {
						fprintf(codegen_fileptr, "elif _opcode == opcode.%s:\n", data->token_name);
					}

					fprintf(codegen_fileptr, "\t\t_data = %s\n", data->token_name);
					fprintf(codegen_fileptr, "\t\t_data.deserialize(ss)\n");
					fprintf(codegen_fileptr, "\t\treturn _data\n");
				}
			}
			fprintf(codegen_fileptr, "\treturn None\n");
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
