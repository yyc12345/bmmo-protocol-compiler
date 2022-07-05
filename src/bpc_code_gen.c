#include "bpc_code_gen.h"
#include <stdio.h>
#include <inttypes.h>
#include "bpc_encoding.h"
#include "bpc_error.h"

#ifdef G_OS_WIN32
#include <Windows.h>
#endif

//const uint32_t bpc_codegen_basic_type_size[] = {
//	4, 8, 1, 2, 4, 8, 1, 2, 4, 8
//};

static const char* bpc_codegen_python_struct_pattern[] = {
	"f", "d", "b", "h", "i", "q", "B", "H", "I", "Q"
};
static const char* bpc_codegen_python_struct_pattern_len[] = {
	"4", "8", "1", "2", "4", "8", "1", "2", "4", "8"
};


static FILE* fs_python = NULL;
static FILE* fs_csharp = NULL;
static FILE* fs_cpp_hdr = NULL;
static FILE* fs_cpp_src = NULL;
static FILE* fs_proto = NULL;
static GSList* token_registery = NULL;
static uint32_t msg_index_distributor = 0;

#define BPC_CODEGEN_OUTPUT_PYTHON (fs_python != NULL)
#define BPC_CODEGEN_OUTPUT_CSHARP (fs_csharp != NULL)
#define BPC_CODEGEN_OUTPUT_CPP_H (fs_cpp_hdr != NULL)
#define BPC_CODEGEN_OUTPUT_CPP_C (fs_cpp_src != NULL)
#define BPC_CODEGEN_OUTPUT_PROTO (fs_proto != NULL)

#define BPC_CODEGEN_INDENT_INIT uint32_t _indent_level = 0, _indent_loop = 0; FILE* _indent_fs = NULL;
#define BPC_CODEGEN_INDENT_RESET(fs) _indent_level = _indent_loop = 0; _indent_fs = fs;
#define BPC_CODEGEN_INDENT_INC ++_indent_level;
#define BPC_CODEGEN_INDENT_DEC --_indent_level;
#define BPC_CODEGEN_INDENT_PRINT fputc('\n', _indent_fs); for(_indent_loop=0;_indent_loop<_indent_level;++_indent_loop) fputc('\t', _indent_fs);

void bpcgen_init_code_file(BPCCMD_PARSED_ARGS* bpc_args) {
	fs_python = bpc_args->out_python_file;
	fs_csharp = bpc_args->out_csharp_file;
	fs_cpp_hdr = bpc_args->out_cpp_header_file;
	fs_cpp_src = bpc_args->out_cpp_source_file;
	fs_proto = bpc_args->out_proto_file;
}

void bpcgen_write_document(BPCSMTV_DOCUMENT* document) {
	// init some essential variables
	_bpcgen_token_registery_reset();
	msg_index_distributor = 0;

	// write preset code
	_bpcgen_write_preset_code(document->namespace_data);

	// then write data according to its type
	GSList* cursor;
	BPCSMTV_DEFINE_GROUP* data;
	for (cursor = document->define_group_data; cursor != NULL; cursor = cursor->next) {
		data = (BPCSMTV_DEFINE_GROUP*)cursor->data;
		switch (data->node_type) {
			case BPCSMTV_DEFINED_TOKEN_TYPE_ALIAS:
				_bpcgen_write_alias(data->node_data.alias_data);
				break;
			case BPCSMTV_DEFINED_TOKEN_TYPE_ENUM:
				_bpcgen_write_enum(data->node_data.enum_data);
				break;
			case BPCSMTV_DEFINED_TOKEN_TYPE_STRUCT:
				_bpcgen_write_struct(data->node_data.struct_data);
				break;
			case BPCSMTV_DEFINED_TOKEN_TYPE_MSG:
				_bpcgen_write_msg(data->node_data.msg_data);
				break;
		}
	}

	// write opcode
	_bpcgen_write_opcode();
}

void bpcgen_free_code_file() {
	// just empty file stream variables in this file.
	// the work of file free is taken by commandline parser.
	fs_python = fs_csharp = fs_cpp_hdr = fs_cpp_src = fs_proto = NULL;
}

void _bpcgen_token_registery_reset() {
	GSList* cursor;
	for (cursor = token_registery; cursor != NULL; cursor = cursor->next) {
		g_free(((BPCGEN_TOKEN_REGISTERY_ENTRY*)cursor->data)->token_name);
	}
	g_slist_free(token_registery);
}

BPCGEN_TOKEN_REGISTERY_ENTRY* _bpcgen_token_registery_get(const char* token_name) {
	GSList* cursor;
	for (cursor = token_registery; cursor != NULL; cursor = cursor->next) {
		BPCGEN_TOKEN_REGISTERY_ENTRY* data = (BPCGEN_TOKEN_REGISTERY_ENTRY*)cursor->data;
		if (g_str_equal(token_name, data->token_name)) return data;
	}
	return NULL;
}

void _bpcgen_write_alias(BPCSMTV_ALIAS* token_data) {
	// alloc entry and push into list
	BPCGEN_TOKEN_REGISTERY_ENTRY* entry = g_new0(BPCGEN_TOKEN_REGISTERY_ENTRY, 1);
	entry->token_name = g_strdup(token_data->user_type);
	entry->token_type = BPCSMTV_DEFINED_TOKEN_TYPE_ALIAS;
	entry->token_basic_type = token_data->basic_type;
	token_registery = g_slist_append(token_registery, entry);

	// ==================== Language Output ====================
	if (BPC_CODEGEN_OUTPUT_PYTHON) {
		;
		// do nothing, because python is weak-type lang.
		// each member do not declare its type expilct.
		// so alias is useless
	}
	if (BPC_CODEGEN_OUTPUT_CSHARP) {
		;
	}
	if (BPC_CODEGEN_OUTPUT_CPP_H) {
		;
	}
	if (BPC_CODEGEN_OUTPUT_CPP_C) {
		;
	}
	if (BPC_CODEGEN_OUTPUT_PROTO) {
		;
	}
}

void _bpcgen_write_enum(BPCSMTV_ENUM* token_data) {
	// alloc entry and push into list
	BPCGEN_TOKEN_REGISTERY_ENTRY* entry = g_new0(BPCGEN_TOKEN_REGISTERY_ENTRY, 1);
	entry->token_name = g_strdup(token_data->enum_name);
	entry->token_type = BPCSMTV_DEFINED_TOKEN_TYPE_ENUM;
	entry->token_basic_type = token_data->enum_basic_type;
	token_registery = g_slist_append(token_registery, entry);

	// define some useful variable
	BPC_CODEGEN_INDENT_INIT;
	GSList* cursor;
	int64_t counter = 0;

	// ==================== Language Output ====================
	if (BPC_CODEGEN_OUTPUT_PYTHON) {
		BPC_CODEGEN_INDENT_RESET(fs_python);
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_python, "class %s():", token_data->enum_name); BPC_CODEGEN_INDENT_INC;
		for (cursor = token_data->enum_body; cursor != NULL; cursor = cursor->next) {
			// check whether entry have spec value
			BPCSMTV_ENUM_BODY* data = (BPCSMTV_ENUM_BODY*)cursor->data;
			if (data->have_specific_value) {
				counter = data->specific_value;
			}

			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_python, "%s = %" PRIi64, data->enum_name, counter++);
		}
		// enum is over
		BPC_CODEGEN_INDENT_DEC;
	}
	if (BPC_CODEGEN_OUTPUT_CSHARP) {
		;
	}
	if (BPC_CODEGEN_OUTPUT_CPP_H) {
		;
	}
	if (BPC_CODEGEN_OUTPUT_CPP_C) {
		;
	}
	if (BPC_CODEGEN_OUTPUT_PROTO) {
		;
	}
}

void _bpcgen_write_struct(BPCSMTV_STRUCT* token_data) {
	_bpcgen_gen_struct_msg_body(token_data->struct_name, token_data->struct_member, NULL);
}
void _bpcgen_write_msg(BPCSMTV_MSG* token_data) {
	BPCGEN_MSG_EXTRA_PROPS props{};
	props.is_reliable = token_data->is_reliable;
	_bpcgen_gen_struct_msg_body(token_data->msg_name, token_data->msg_member, &props);
}
void _bpcgen_gen_struct_msg_body(const char* token_name, GSList* member_list, BPCGEN_MSG_EXTRA_PROPS* msg_prop) {
	bool is_msg = msg_prop != NULL;

	// generate msg index
	uint32_t msg_index = 0;
	if (is_msg) msg_index = msg_index_distributor++;

	// alloc entry and push into list
	BPCGEN_TOKEN_REGISTERY_ENTRY* entry = g_new0(BPCGEN_TOKEN_REGISTERY_ENTRY, 1);
	entry->token_name = g_strdup(token_name);
	if (is_msg) {
		entry->token_type = BPCSMTV_DEFINED_TOKEN_TYPE_MSG;
		entry->token_arranged_index = msg_index;
	} else {
		entry->token_type = BPCSMTV_DEFINED_TOKEN_TYPE_STRUCT;
	}
	token_registery = g_slist_append(token_registery, entry);

	// define some useful variable
	BPC_CODEGEN_INDENT_INIT;
	GSList* cursor;
	GString* operator_name = g_string_new(NULL);
	bool proc_like_basic_type;
	BPCSMTV_BASIC_TYPE underlaying_basic_type;

	// ==================== Language Output ====================
	if (BPC_CODEGEN_OUTPUT_PYTHON) {
		BPC_CODEGEN_INDENT_RESET(fs_python);

		// generate header and props
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_python, "class %s():", token_name); BPC_CODEGEN_INDENT_INC;
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_python, "def __init__(self):");

		BPC_CODEGEN_INDENT_INC;
		for (cursor = member_list; cursor != NULL; cursor = cursor->next) {
			BPCSMTV_MEMBER* data = (BPCSMTV_MEMBER*)cursor->data;

			BPC_CODEGEN_INDENT_PRINT;
			if (data->array_prop.is_array) {
				fprintf(fs_python, "self.%s = []", data->vname);
			} else {
				fprintf(fs_python, "self.%s = None", data->vname);
			}
		}
		if (member_list == NULL) {
			// if there are no member, wee need write extra `pass`
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_python, "pass");
		}
		BPC_CODEGEN_INDENT_DEC;

		// generate reliable getter and opcode getter for msg
		if (is_msg) {
			// reliable
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_python, "def get_reliable(self) -> bool:"); BPC_CODEGEN_INDENT_INC;

			BPC_CODEGEN_INDENT_PRINT;
			if (msg_prop->is_reliable) fprintf(fs_python, "return True");
			else fprintf(fs_python, "return False");
			BPC_CODEGEN_INDENT_DEC;

			// opcode
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_python, "def get_opcode(self) -> int:");

			BPC_CODEGEN_INDENT_INC;
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_python, "return %d", msg_index);
			BPC_CODEGEN_INDENT_DEC;
		}

		// generate deserialize func
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_python, "def deserialize(self, ss: io.BytesIO):"); BPC_CODEGEN_INDENT_INC;
		if (is_msg) {
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_python, "if struct.unpack('I', ss.read(4))[0] != %d:", msg_index); BPC_CODEGEN_INDENT_INC;
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_python, "raise Exception('invalid opcode!')"); BPC_CODEGEN_INDENT_DEC;
		}
		for (cursor = member_list; cursor != NULL; cursor = cursor->next) {
			BPCSMTV_MEMBER* data = (BPCSMTV_MEMBER*)cursor->data;

			// annotation
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_python, "# %s", data->vname);

			// if member is array, we need construct a loop
			if (data->array_prop.is_array) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "self.%s.clear()", data->vname);
				if (data->array_prop.is_static_array) {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "for _i in range(%d):", data->array_prop.array_len);
				} else {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "_count = struct.unpack('I', ss.read(4))[0]");
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "for _i in range(_count):");
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
			_bpcgen_get_underlaying_type(data, &proc_like_basic_type, &underlaying_basic_type);
			// determine read method
			if (proc_like_basic_type) {
				// basic type deserialize
				if (underlaying_basic_type == BPCSMTV_BASIC_TYPE_STRING) {
					// string is special
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "_strlen = struct.unpack('I', ss.read(4))[0]");
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "%s = ss.read(_strlen).decode(encoding='gb2312', errors='ignore')", operator_name->str);
				} else {
					// other value type can use table to generate
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "%s = struct.unpack('%s', ss.read(%s))[0]",
						operator_name->str,
						bpc_codegen_python_struct_pattern[(size_t)underlaying_basic_type],
						bpc_codegen_python_struct_pattern_len[(size_t)underlaying_basic_type]);
				}
			} else {
				// struct deserialize
				// call struct.deserialize()
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "%s = %s()", operator_name->str, data->v_struct_type);
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "%s.deserialize(ss)", operator_name->str);
			}
			// compute align data
			if (data->align_prop.use_align) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "ss.read(%d)", data->align_prop.padding_size);
			}

			// array extra proc
			if (data->array_prop.is_array) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "self.%s.append(_cache)", data->vname);

				// shink indent
				BPC_CODEGEN_INDENT_DEC;
			}
		}
		if (member_list == NULL && (!is_msg)) {
			// if there are no member, and this struct is not msg,
			// we need write extra `pass`
			// because msg have at least one entry `opcode` so it doesn't need pass
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_python, "pass");
		}
		// func deserialize is over
		BPC_CODEGEN_INDENT_DEC;

		// generate serialize func
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_python, "def serialize(self, ss: io.BytesIO):"); BPC_CODEGEN_INDENT_INC;
		if (is_msg) {
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_python, "ss.write(struct.pack('I', %d))", msg_index);
		}
		for (cursor = member_list; cursor != NULL; cursor = cursor->next) {
			BPCSMTV_MEMBER* data = (BPCSMTV_MEMBER*)cursor->data;

			// annotation
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_python, "# %s", data->vname);

			// if member is array, we need construct a loop
			if (data->array_prop.is_array) {
				// increase indent level for array
				if (data->array_prop.is_static_array) {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "for _i in range(%d):", data->array_prop.array_len); BPC_CODEGEN_INDENT_INC;
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "_item = self.%s[_i]", data->vname);
				} else {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "ss.write('I', len(self.%s))", data->vname);
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "for _item in self.%s:", data->vname); BPC_CODEGEN_INDENT_INC;
				}
			}

			// determine operator name
			if (data->array_prop.is_array) {
				g_string_printf(operator_name, "_item");
			} else {
				g_string_printf(operator_name, "self.%s", data->vname);
			}

			// determine read method
			_bpcgen_get_underlaying_type(data, &proc_like_basic_type, &underlaying_basic_type);
			if (proc_like_basic_type) {
				// basic type serialize
				if (underlaying_basic_type == BPCSMTV_BASIC_TYPE_STRING) {
					// string is special in basic type
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "_binstr = %s.encode(encoding='gb2312', errors='ignore')", operator_name->str);
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "ss.write(struct.pack('I', len(_binstr)))");
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "ss.write(_binstr)");
				} else {
					// other value can be generated by table search
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "ss.write(struct.pack('%s', %s))",
						bpc_codegen_python_struct_pattern[(size_t)underlaying_basic_type],
						operator_name->str);
				}
			} else {
				// struct serialize
				// call struct.serialize()
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "%s.serialize(ss)", operator_name->str);
			}
			// compute align size
			if (data->align_prop.use_align) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "ss.write(b'\\0' * %d)", data->align_prop.padding_size);
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
			fprintf(fs_python, "pass");
		}
		// func deserialize is over
		BPC_CODEGEN_INDENT_DEC;

		// class define is over
		BPC_CODEGEN_INDENT_DEC;

	}
	if (BPC_CODEGEN_OUTPUT_CSHARP) {
		;
	}
	if (BPC_CODEGEN_OUTPUT_CPP_H) {
		;
	}
	if (BPC_CODEGEN_OUTPUT_CPP_C) {
		;
	}
	if (BPC_CODEGEN_OUTPUT_PROTO) {
		;
	}

	g_string_free(operator_name, TRUE);
}

void _bpcgen_write_preset_code(GSList* namespace_list) {
	// setup template
	if (BPC_CODEGEN_OUTPUT_PYTHON) {
		_bpcgen_copy_template(fs_python, u8"snippets/header.py");
	}
	if (BPC_CODEGEN_OUTPUT_CSHARP) {
		bpcerr_warning(BPCERR_ERROR_SOURCE_CODEGEN, "Csharp code gen is unsupported now. It will come soon.");
	}
	if (BPC_CODEGEN_OUTPUT_CPP_H) {
		bpcerr_warning(BPCERR_ERROR_SOURCE_CODEGEN, "Cpp code gen is unsupported now. It will come soon.");
	}
	if (BPC_CODEGEN_OUTPUT_CPP_C) {
		bpcerr_warning(BPCERR_ERROR_SOURCE_CODEGEN, "Cpp code gen is unsupported now. It will come soon.");
	}
	if (BPC_CODEGEN_OUTPUT_PROTO) {
		bpcerr_warning(BPCERR_ERROR_SOURCE_CODEGEN, "Proto gen is unsupported now. It will come soon.");
	}
}

void _bpcgen_write_opcode() {
	// define useful variable
	BPC_CODEGEN_INDENT_INIT;
	GSList* cursor;

	// ==================== Language Output ====================
	if (BPC_CODEGEN_OUTPUT_PYTHON) {
		BPC_CODEGEN_INDENT_RESET(fs_python);

		// write opcode enum
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_python, "class opcode():"); BPC_CODEGEN_INDENT_INC;
		for (cursor = token_registery; cursor != NULL; cursor = cursor->next) {
			BPCGEN_TOKEN_REGISTERY_ENTRY* data = (BPCGEN_TOKEN_REGISTERY_ENTRY*)cursor->data;

			if (data->token_type == BPCSMTV_DEFINED_TOKEN_TYPE_MSG) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "%s = %d", data->token_name, data->token_arranged_index);
			}
		}
		// class opcode is over
		BPC_CODEGEN_INDENT_DEC;

		// write uniformed deserialize func
		bool is_first = true;
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_python, "def uniform_deserialize(ss: io.BytesIO):"); BPC_CODEGEN_INDENT_INC;
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_python, "_opcode = _peek_opcode(ss)");
		for (cursor = token_registery; cursor != NULL; cursor = cursor->next) {
			BPCGEN_TOKEN_REGISTERY_ENTRY* data = (BPCGEN_TOKEN_REGISTERY_ENTRY*)cursor->data;

			if (data->token_type == BPCSMTV_DEFINED_TOKEN_TYPE_MSG) {
				BPC_CODEGEN_INDENT_PRINT;
				if (is_first) {
					fprintf(fs_python, "if _opcode == opcode.%s:", data->token_name);
					is_first = false;
				} else {
					fprintf(fs_python, "elif _opcode == opcode.%s:", data->token_name);
				}

				// write if body
				BPC_CODEGEN_INDENT_INC;
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "_data = %s()", data->token_name);
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "_data.deserialize(ss)");
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "return _data");
				BPC_CODEGEN_INDENT_DEC;
			}
		}
		// default return
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_python, "return None");
		// uniform func is over
		BPC_CODEGEN_INDENT_DEC;

	}
	if (BPC_CODEGEN_OUTPUT_CSHARP) {
		;
	}
	if (BPC_CODEGEN_OUTPUT_CPP_H) {
		;
	}
	if (BPC_CODEGEN_OUTPUT_CPP_C) {
		;
	}
	if (BPC_CODEGEN_OUTPUT_PROTO) {
		;
	}
}

//uint32_t _bpc_codegen_get_align_padding_size(BPCSMTV_MEMBER* token, BPCSMTV_BASIC_TYPE underlaying_type) {
//	if (!token->align_prop.use_align) return 0;
//	int32_t original_size = bpc_codegen_basic_type_size[underlaying_type];
//	int32_t padding_size = token->align_prop.expected_size - original_size;
//	if (padding_size > 0) return (uint32_t)padding_size;
//	else return 0u;
//}

char* _bpcgen_get_dot_style_namespace(GSList* namespace_list) {
	GString* strl = g_string_new(NULL);
	GSList* cursor;
	for (cursor = namespace_list; cursor != NULL; cursor = cursor->next) {
		g_string_append(strl, cursor->data);
		if (cursor->next != NULL)
			g_string_append_c(strl, '.');
	}

	return g_string_free(strl, false);
}

void _bpcgen_get_underlaying_type(BPCSMTV_MEMBER* token, bool* pout_proc_like_basic_type, BPCSMTV_BASIC_TYPE* pout_underlaying_basic_type) {
	if (token->is_basic_type) {
		*pout_proc_like_basic_type = true;
		*pout_underlaying_basic_type = token->v_basic_type;
	} else {
		// check user defined type
		// token type will never be msg accoring to syntax define
		BPCGEN_TOKEN_REGISTERY_ENTRY* entry = _bpcgen_token_registery_get(token->v_struct_type);
		if (entry->token_type == BPCSMTV_DEFINED_TOKEN_TYPE_STRUCT) {
			// struct
			*pout_proc_like_basic_type = false;
		} else {
			// alias and enum
			*pout_proc_like_basic_type = true;
			*pout_underlaying_basic_type = entry->token_basic_type;
		}
	}
}

void _bpcgen_copy_template(FILE* target, const char* u8_template_code_file_path) {
	// first, we need get where our program locate

	// store program location with glib fs format
	// use static for pernament storage
	static gchar* prgloc = NULL;
	// store the path combine dir name and specific file
	// stored as glib fs format
	gchar* combined_path = NULL;

	// first run, get it
	if (prgloc == NULL) {
		int bytes;
		size_t len = 1024;
		gchar* utf8_prgloc = NULL;
#ifdef G_OS_WIN32
		wchar_t pBuf[1024];
		bytes = GetModuleFileNameW(NULL, pBuf, len);
		if (bytes)
			prgloc = bpcenc_wchar_to_glibfs(pBuf);
#else
		char pBuf[1024];
		bytes = MIN(readlink("/proc/self/exe", pBuf, len), len - 1);
		if (bytes >= 0)
			pBuf[bytes] = '\0';

		// read from linux is utf8 string, use it directly
		prgloc = bpcenc_utf8_to_glibfs(pBuf);
#endif
	}

	// combine user specific path with program path
	if (prgloc != NULL) {
		gchar* dir_name = NULL;
		gchar* file_name = NULL;

		dir_name = g_path_get_dirname(prgloc);
		file_name = bpcenc_utf8_to_glibfs(u8_template_code_file_path);
		if (dir_name != NULL && file_name != NULL) {
			combined_path = g_build_filename(dir_name, file_name, NULL);
		}

		g_free(dir_name);
		g_free(file_name);
	} else {
		// failed. raise warning
		goto template_warning;
	}

	// open file and copy it
	if (combined_path != NULL) {
		FILE* template_fs = NULL;
#ifdef G_OS_WIN32
		// in windows, we need convert it into wstring to open file
		wchar_t* wfilename = NULL;
		wfilename = bpcenc_glibfs_to_wchar(combined_path);
		if (wfilename != NULL)
			template_fs = _wfopen(wfilename, L"r+");
		g_free(wfilename);
#else
		// in linux, just open it directly
		gchar* u8filename = NULL;
		u8filename = bpcenc_glibfs_to_utf8(combined_path);
		if (u8filename != NULL)
			template_fs = fopen(u8filename, "r+");
		g_free(u8filename);
#endif

		// check validation
		if (template_fs == NULL)
			goto template_warning;

		_bpcgen_copy_file_stream(target, template_fs);
		fclose(template_fs);
	} else {
		// failed. raise warning
		goto template_warning;
	}

	g_free(combined_path);
	return;

template_warning:
	g_free(combined_path);
	bpcerr_warning(BPCERR_ERROR_SOURCE_CODEGEN, "Fail to open code template file: %s\n\
		Code generation will continue, but generated code may not work.", u8_template_code_file_path);
	return;
}

void _bpcgen_copy_file_stream(FILE* target, FILE* fs) {
	char* buffer = g_new0(char, 1024);
	size_t read_counter;

	while ((read_counter = fread(buffer, sizeof(char), 1024, fs)) != 0) {
		fwrite(buffer, sizeof(char), read_counter, target);
	}
}
