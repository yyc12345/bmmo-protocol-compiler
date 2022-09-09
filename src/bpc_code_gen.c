#include "bpc_code_gen.h"
#include <stdio.h>
#include <inttypes.h>
#include "bpc_error.h"
#include "bpc_fs.h"

//const uint32_t bpc_codegen_basic_type_size[] = {
//	4, 8, 1, 2, 4, 8, 1, 2, 4, 8
//};

static const char* code_python_struct_pattern[] = {
	"f", "d", "b", "h", "i", "q", "B", "H", "I", "Q"
};
static const char* code_python_struct_pattern_len[] = {
	"4", "8", "1", "2", "4", "8", "1", "2", "4", "8"
};

static const char* code_csharp_basic_type[] = {
	"float", "double", "sbyte", "short", "int", "long", "byte", "ushort", "uint", "ulong", "string"
};
static const char* code_csharp_formal_basic_type[] = {
	"Single", "Double", "SByte", "Int16", "Int32", "Int64", "Byte", "UInt16", "UInt32", "UInt64", "String"
};

static const char* code_cpp_basic_type[] = {
	"float", "double", "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "std::string"
};


static FILE* fs_python = NULL;
static FILE* fs_csharp = NULL;
static FILE* fs_cpp_hdr = NULL;
static FILE* fs_cpp_src = NULL;
static FILE* fs_proto = NULL;
static gchar* hpp_reference = NULL;

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

	hpp_reference = g_strdup(bpc_args->ref_cpp_relative_hdr->str);
}

void bpcgen_write_document(BPCSMTV_DOCUMENT* document) {
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

	// write conclusion code, such as opcode and uniform deserializer
	_bpcgen_write_conclusion_code();

	// write tail
	_bpcgen_write_tail_code(document->namespace_data);
}

void bpcgen_free_code_file() {
	// just empty file stream variables in this file.
	// the work of file free is taken by commandline parser.
	fs_python = fs_csharp = fs_cpp_hdr = fs_cpp_src = fs_proto = NULL;
	g_free(hpp_reference);
}

void _bpcgen_write_alias(BPCSMTV_ALIAS* token_data) {
	BPC_CODEGEN_INDENT_INIT;

	// ==================== Language Output ====================
	if (BPC_CODEGEN_OUTPUT_PYTHON) {
		;
		// do nothing, because python is weak-type lang.
		// each member do not declare its type expilct.
		// so alias is useless
	}
	if (BPC_CODEGEN_OUTPUT_CSHARP) {
		;
		// do nothing
		// although c# have syntax: using xxx = yyy;
		// but it only can be used in new c# version(higher than c# 8.0)
		// so i give up using this features.
	}
	if (BPC_CODEGEN_OUTPUT_CPP_H) {
		BPC_CODEGEN_INDENT_RESET(fs_cpp_hdr);
		fprintf(fs_cpp_hdr, "typedef %s %s\n", code_cpp_basic_type[token_data->basic_type], token_data->user_type);
	}
	if (BPC_CODEGEN_OUTPUT_CPP_C) {
		;
		// do nothing
		// typedef has been written in header file
		// so source file do not need write it
	}
	if (BPC_CODEGEN_OUTPUT_PROTO) {
		;
		// do nothing
		// proto do not have alias features
	}
}

void _bpcgen_write_enum(BPCSMTV_ENUM* token_data) {
	// define some useful variable
	BPC_CODEGEN_INDENT_INIT;
	GSList* cursor;
	int64_t counter = 0;

	// ==================== Language Output ====================
	if (BPC_CODEGEN_OUTPUT_PYTHON) {
		BPC_CODEGEN_INDENT_RESET(fs_python);
		counter = 0;

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
		BPC_CODEGEN_INDENT_RESET(fs_csharp);
		counter = 0;

		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_csharp, "public enum %s : %s {", token_data->enum_name, code_csharp_basic_type[token_data->enum_basic_type]); BPC_CODEGEN_INDENT_INC;
		for (cursor = token_data->enum_body; cursor != NULL; cursor = cursor->next) {
			BPCSMTV_ENUM_BODY* data = (BPCSMTV_ENUM_BODY*)cursor->data;
			if (data->have_specific_value) {
				counter = data->specific_value;
			}

			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_csharp, "%s = %" PRIi64, data->enum_name, counter++);
			if (cursor->next != NULL) {
				fputs(",", fs_csharp);
			}
		}
		// enum is over
		BPC_CODEGEN_INDENT_DEC;
		BPC_CODEGEN_INDENT_PRINT;
		fputs("}", fs_csharp);
	}
	if (BPC_CODEGEN_OUTPUT_CPP_H) {
		BPC_CODEGEN_INDENT_RESET(fs_cpp_hdr);
		counter = 0;

		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_cpp_hdr, "enum class %s : %s {", token_data->enum_name, code_cpp_basic_type[token_data->enum_basic_type]); BPC_CODEGEN_INDENT_INC;
		for (cursor = token_data->enum_body; cursor != NULL; cursor = cursor->next) {
			BPCSMTV_ENUM_BODY* data = (BPCSMTV_ENUM_BODY*)cursor->data;
			if (data->have_specific_value) {
				counter = data->specific_value;
			}

			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_cpp_hdr, "%s = %" PRIi64, data->enum_name, counter++);
			if (cursor->next != NULL) {
				fputs(",", fs_cpp_hdr);
			}
		}
		// enum is over
		BPC_CODEGEN_INDENT_DEC;
		BPC_CODEGEN_INDENT_PRINT;
		fputs("};", fs_cpp_hdr);
	}
	if (BPC_CODEGEN_OUTPUT_CPP_C) {
		;
		// do nothing
		// cpp enum define has been written in header file
		// source file do not need declare it anymore.
	}
	if (BPC_CODEGEN_OUTPUT_PROTO) {
		;
	}
}

void _bpcgen_write_struct(BPCSMTV_STRUCT* token_data) {
	_bpcgen_gen_struct_msg_body(token_data->struct_name, token_data->struct_member, NULL);
}
void _bpcgen_write_msg(BPCSMTV_MSG* token_data) {
	BPCGEN_MSG_EXTRA_PROPS props;
	props.is_reliable = token_data->is_reliable;
	_bpcgen_gen_struct_msg_body(token_data->msg_name, token_data->msg_member, &props);
}
void _bpcgen_gen_struct_msg_body(const char* token_name, GSList* smtv_member_list, BPCGEN_MSG_EXTRA_PROPS* msg_prop) {
	bool is_msg = msg_prop != NULL;

	// get msg index
	BPCSMTV_TOKEN_REGISTERY_ITEM* token_instance = bpcsmtv_token_registery_get(token_name);
	uint32_t msg_index = token_instance->token_extra_props.token_arranged_index;

	// define some useful variable
	BPC_CODEGEN_INDENT_INIT;
	GSList* cursor;
	GString* declare_oper_name = g_string_new(NULL);
	GString* reference_oper_name = g_string_new(NULL);

	// we need compute underlaying type for each item
	guint member_count = g_slist_length(smtv_member_list);
	GSList* codegen_member_list = NULL;
	for (cursor = smtv_member_list; cursor != NULL; cursor = cursor->next) {
		BPCGEN_UNDERLAYING_MEMBER* member = g_new0(BPCGEN_UNDERLAYING_MEMBER, 1);
		codegen_member_list = g_slist_append(codegen_member_list, member);

		member->semantic_value = (BPCSMTV_MEMBER*)cursor->data;
		_bpcgen_get_underlaying_type(member);
	}

	// ==================== Language Output ====================
	if (BPC_CODEGEN_OUTPUT_PYTHON) {
		BPC_CODEGEN_INDENT_RESET(fs_python);

		// generate header and props
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_python, "class %s():", token_name); BPC_CODEGEN_INDENT_INC;
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_python, "def __init__(self):");

		BPC_CODEGEN_INDENT_INC;
		for (cursor = codegen_member_list; cursor != NULL; cursor = cursor->next) {
			BPCGEN_UNDERLAYING_MEMBER* data = (BPCGEN_UNDERLAYING_MEMBER*)cursor->data;

			BPC_CODEGEN_INDENT_PRINT;
			if (data->semantic_value->array_prop.is_array) {
				fprintf(fs_python, "self.%s = []", data->semantic_value->vname);
			} else {
				fprintf(fs_python, "self.%s = None", data->semantic_value->vname);
			}
		}
		if (codegen_member_list == NULL) {
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
		for (cursor = codegen_member_list; cursor != NULL; cursor = cursor->next) {
			BPCGEN_UNDERLAYING_MEMBER* data = (BPCGEN_UNDERLAYING_MEMBER*)cursor->data;

			// annotation
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_python, "# %s", data->semantic_value->vname);

			// if member is array, we need construct a loop
			if (data->semantic_value->array_prop.is_array) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "self.%s.clear()", data->semantic_value->vname);
				if (data->semantic_value->array_prop.is_static_array) {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "for _i in range(%d):", data->semantic_value->array_prop.array_len);
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
			if (data->semantic_value->array_prop.is_array) {
				g_string_printf(declare_oper_name, "_cache");
				g_string_printf(reference_oper_name, "_cache");
			} else {
				g_string_printf(declare_oper_name, "self.%s", data->semantic_value->vname);
				g_string_printf(reference_oper_name, "self.%s", data->semantic_value->vname);
			}

			// resolve basic type
			// determine read method
			if (data->like_basic_type) {
				// basic type deserialize
				if (data->underlaying_basic_type == BPCSMTV_BASIC_TYPE_STRING) {
					// string is special
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "%s = _read_bmmo_string(ss)", declare_oper_name->str);
				} else {
					// other value type can use table to generate
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "%s = struct.unpack('%s', ss.read(%s))[0]",
						declare_oper_name->str,
						code_python_struct_pattern[(size_t)data->underlaying_basic_type],
						code_python_struct_pattern_len[(size_t)data->underlaying_basic_type]);
				}
			} else {
				// struct deserialize
				// call struct.deserialize()
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "%s = %s()", declare_oper_name->str, data->semantic_value->v_struct_type);
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "%s.deserialize(ss)", reference_oper_name->str);
			}

			// array extra proc
			if (data->semantic_value->array_prop.is_array) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "self.%s.append(_cache)", data->semantic_value->vname);

				// shink indent
				BPC_CODEGEN_INDENT_DEC;
			}

			// compute align data
			if (data->semantic_value->align_prop.use_align) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "ss.read(%d)", data->semantic_value->align_prop.padding_size);
			}
		}
		if (codegen_member_list == NULL && (!is_msg)) {
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
		for (cursor = codegen_member_list; cursor != NULL; cursor = cursor->next) {
			BPCGEN_UNDERLAYING_MEMBER* data = (BPCGEN_UNDERLAYING_MEMBER*)cursor->data;

			// annotation
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_python, "# %s", data->semantic_value->vname);

			// if member is array, we need construct a loop
			if (data->semantic_value->array_prop.is_array) {
				// increase indent level for array
				if (data->semantic_value->array_prop.is_static_array) {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "for _i in range(%d):", data->semantic_value->array_prop.array_len); BPC_CODEGEN_INDENT_INC;
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "_item = self.%s[_i]", data->semantic_value->vname);
				} else {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "ss.write('I', len(self.%s))", data->semantic_value->vname);
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "for _item in self.%s:", data->semantic_value->vname); BPC_CODEGEN_INDENT_INC;
				}
			}

			// determine operator name
			if (data->semantic_value->array_prop.is_array) {
				g_string_printf(reference_oper_name, "_item");
			} else {
				g_string_printf(reference_oper_name, "self.%s", data->semantic_value->vname);
			}

			// determine read method
			if (data->like_basic_type) {
				// basic type serialize
				if (data->underlaying_basic_type == BPCSMTV_BASIC_TYPE_STRING) {
					// string is special in basic type
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "_write_bmmo_string(ss, %s)", reference_oper_name->str);
				} else {
					// other value can be generated by table search
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_python, "ss.write(struct.pack('%s', %s))",
						code_python_struct_pattern[(size_t)data->underlaying_basic_type],
						reference_oper_name->str);
				}
			} else {
				// struct serialize
				// call struct.serialize()
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "%s.serialize(ss)", reference_oper_name->str);
			}

			// array extra proc
			if (data->semantic_value->array_prop.is_array) {
				// shink indent
				BPC_CODEGEN_INDENT_DEC;
			}

			// compute align size
			if (data->semantic_value->align_prop.use_align) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "ss.write(b'\\0' * %d)", data->semantic_value->align_prop.padding_size);
			}
		}
		if (codegen_member_list == NULL && (!is_msg)) {
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
		BPC_CODEGEN_INDENT_RESET(fs_csharp);
		const gchar* cs_type_str = NULL;

		// class decleartion
		BPC_CODEGEN_INDENT_PRINT;
		if (is_msg) {
			fprintf(fs_csharp, "public class %s : _BpcMessage {", token_name);
		} else {
			fprintf(fs_csharp, "public class %s {", token_name);
		}
		BPC_CODEGEN_INDENT_INC;

		// class member & constructor
		for (cursor = codegen_member_list; cursor != NULL; cursor = cursor->next) {
			BPCGEN_UNDERLAYING_MEMBER* data = (BPCGEN_UNDERLAYING_MEMBER*)cursor->data;
			if (data->is_pure_basic_type) {
				cs_type_str = code_csharp_basic_type[data->underlaying_basic_type];
			} else {
				cs_type_str = data->c_style_type;
			}

			BPC_CODEGEN_INDENT_PRINT;
			if (data->semantic_value->array_prop.is_array) {
				if (data->semantic_value->array_prop.is_static_array) {
					fprintf(fs_csharp, "public %s[] %s = new %s[%d];", cs_type_str, data->semantic_value->vname, cs_type_str, data->semantic_value->array_prop.array_len);
				} else {
					fprintf(fs_csharp, "public List<%s> %s = new List<%s>();", cs_type_str, data->semantic_value->vname, cs_type_str);
				}
			} else {
				fprintf(fs_csharp, "public %s %s;", cs_type_str, data->semantic_value->vname);
			}
		}

		// reliable and opcode
		if (is_msg) {
			// reliable
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_csharp, "public bool IsReliable() {"); BPC_CODEGEN_INDENT_INC;

			BPC_CODEGEN_INDENT_PRINT;
			if (msg_prop->is_reliable) fprintf(fs_csharp, "return true;");
			else fprintf(fs_csharp, "return false;");
			BPC_CODEGEN_INDENT_DEC;
			BPC_CODEGEN_INDENT_PRINT;
			fputc('}', fs_csharp);

			// opcode
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_csharp, "public _OpCode GetOpCode() {");

			BPC_CODEGEN_INDENT_INC;
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_csharp, "return (_OpCode)%d;", msg_index);
			BPC_CODEGEN_INDENT_DEC;
			BPC_CODEGEN_INDENT_PRINT;
			fputc('}', fs_csharp);
		}

		// deserializer
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_csharp, "public void Deserialize(BinaryReader br) {"); BPC_CODEGEN_INDENT_INC;
		if (is_msg) {
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_csharp, "if (br.ReadUInt32() != %d)", msg_index); BPC_CODEGEN_INDENT_INC;
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_csharp, "throw new Exception(\"invalid opcode!\");"); BPC_CODEGEN_INDENT_DEC;
		}
		for (cursor = codegen_member_list; cursor != NULL; cursor = cursor->next) {
			BPCGEN_UNDERLAYING_MEMBER* data = (BPCGEN_UNDERLAYING_MEMBER*)cursor->data;

			// annotation
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_csharp, "// %s", data->semantic_value->vname);

			// process array
			if (data->semantic_value->array_prop.is_array) {
				if (data->semantic_value->array_prop.is_static_array) {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_csharp, "for (UInt32 _i = 0; _i < %d; _i++) {", data->semantic_value->array_prop.array_len);
				} else {
					// only dynamic list need clear
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_csharp, "%s.Clear();", data->semantic_value->vname);
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_csharp, "for (UInt32 _i = br.ReadUInt32(); _i > 0; _i--) {");
				}

				// increase indent
				BPC_CODEGEN_INDENT_INC;
			}

			// determine operator name
			if (data->semantic_value->array_prop.is_array) {
				g_string_printf(declare_oper_name, "var _cache");
				g_string_printf(reference_oper_name, "_cache");
			} else {
				g_string_printf(declare_oper_name, "%s", data->semantic_value->vname);
				g_string_printf(reference_oper_name, "%s", data->semantic_value->vname);
			}

			// determine read method
			if (data->like_basic_type) {
				// basic type deserialize
				if (data->underlaying_basic_type == BPCSMTV_BASIC_TYPE_STRING) {
					// string is special
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_csharp, "%s = br.BpcReadString();", declare_oper_name->str);
				} else {
					// other value type can use table to generate
					BPC_CODEGEN_INDENT_PRINT;
					if (data->is_pure_basic_type) {
						fprintf(fs_csharp, "%s = br.Read%s();",
							declare_oper_name->str,
							code_csharp_formal_basic_type[(size_t)data->underlaying_basic_type]);
					} else {
						// non-pure value need a convertion
						fprintf(fs_csharp, "%s = (%s)br.Read%s();",
							declare_oper_name->str,
							data->c_style_type,
							code_csharp_formal_basic_type[(size_t)data->underlaying_basic_type]);
					}
				}
			} else {
				// struct deserialize
				// call struct.deserialize()
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_csharp, "%s = new %s();", declare_oper_name->str, data->semantic_value->v_struct_type);
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_csharp, "%s.Deserialize(br);", reference_oper_name->str);
			}

			// array extra proc
			if (data->semantic_value->array_prop.is_array) {
				BPC_CODEGEN_INDENT_PRINT;
				if (data->semantic_value->array_prop.is_static_array) {
					fprintf(fs_csharp, "%s[_i] = _cache;", data->semantic_value->vname);
				} else {
					fprintf(fs_csharp, "%s.Add(_cache);", data->semantic_value->vname);
				}

				// shink indent
				BPC_CODEGEN_INDENT_DEC;
				BPC_CODEGEN_INDENT_PRINT;
				fputs("}", fs_csharp);
			}

			// compute align data
			if (data->semantic_value->align_prop.use_align) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_csharp, "br.BaseStream.Seek(%d, SeekOrigin.Current);", data->semantic_value->align_prop.padding_size);
			}

		}
		// deserialize is over
		BPC_CODEGEN_INDENT_DEC;
		BPC_CODEGEN_INDENT_PRINT;
		fputs("}", fs_csharp);

		// generate serialize func
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_csharp, "public void Serialize(BinaryWriter bw) {"); BPC_CODEGEN_INDENT_INC;
		if (is_msg) {
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_csharp, "bw.Write((UInt32)%d);", msg_index);
		}
		for (cursor = codegen_member_list; cursor != NULL; cursor = cursor->next) {
			BPCGEN_UNDERLAYING_MEMBER* data = (BPCGEN_UNDERLAYING_MEMBER*)cursor->data;

			// annotation
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_csharp, "// %s", data->semantic_value->vname);

			// if member is array, we need construct a loop
			if (data->semantic_value->array_prop.is_array) {
				// increase indent level for array
				if (data->semantic_value->array_prop.is_static_array) {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_csharp, "for (UInt32 _i = 0; _i < %d; _i++) {", data->semantic_value->array_prop.array_len); BPC_CODEGEN_INDENT_INC;
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_csharp, "var _item = %s[_i];", data->semantic_value->vname);
				} else {
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_csharp, "bw.Write(%s.Count);", data->semantic_value->vname);
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_csharp, "foreach (var _item in %s) {", data->semantic_value->vname); BPC_CODEGEN_INDENT_INC;
				}
			}

			// determine operator name
			if (data->semantic_value->array_prop.is_array) {
				g_string_printf(reference_oper_name, "_item");
			} else {
				g_string_printf(reference_oper_name, "%s", data->semantic_value->vname);
			}

			// determine read method
			if (data->like_basic_type) {
				// basic type serialize
				if (data->underlaying_basic_type == BPCSMTV_BASIC_TYPE_STRING) {
					// string is special in basic type
					BPC_CODEGEN_INDENT_PRINT;
					fprintf(fs_csharp, "bw.BpcWriteString(%s);", reference_oper_name->str);
				} else {
					// other value can be generated by table search
					BPC_CODEGEN_INDENT_PRINT;
					if (data->is_pure_basic_type) {
						fprintf(fs_csharp, "bw.Write(%s);", reference_oper_name->str);
					} else {
						// non-pure value need a convertion
						fprintf(fs_csharp, "bw.Write((%s)%s);",
							code_csharp_basic_type[data->underlaying_basic_type],
							reference_oper_name->str);
					}
				}
			} else {
				// struct serialize
				// call struct.serialize()
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_csharp, "%s.Serialize(bw);", reference_oper_name->str);
			}

			// array extra proc
			if (data->semantic_value->array_prop.is_array) {
				// shink indent
				BPC_CODEGEN_INDENT_DEC;
				BPC_CODEGEN_INDENT_PRINT;
				fputs("}", fs_csharp);
			}

			// compute align size
			if (data->semantic_value->align_prop.use_align) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_csharp, "bw.BaseStream.Seek(%d, SeekOrigin.Current);", data->semantic_value->align_prop.padding_size);
			}
		}
		// serialize is over
		BPC_CODEGEN_INDENT_DEC;
		BPC_CODEGEN_INDENT_PRINT;
		fputs("}", fs_csharp);

		// class define is over
		BPC_CODEGEN_INDENT_DEC;
		BPC_CODEGEN_INDENT_PRINT;
		fputs("}", fs_csharp);

	}
	if (BPC_CODEGEN_OUTPUT_CPP_H) {
		BPC_CODEGEN_INDENT_RESET(fs_cpp_hdr);
		const gchar* cpp_type_str = NULL;

		// class decleartion
		BPC_CODEGEN_INDENT_PRINT;
		if (is_msg) {
			fprintf(fs_cpp_hdr, "class %s : public _BpcMessage {", token_name);
		} else {
			fprintf(fs_cpp_hdr, "class %s {", token_name);
		}
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_cpp_hdr, "public:");
		BPC_CODEGEN_INDENT_INC;

		// constructor & deconstructor declare
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_cpp_hdr, "%s();", token_name);
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_cpp_hdr, "virtual ~%s();", token_name);

		// class member
		// cpp can process alias correctly
		// so we do not need to convert it.
		BPC_CODEGEN_INDENT_PRINT;
		for (cursor = codegen_member_list; cursor != NULL; cursor = cursor->next) {
			BPCGEN_UNDERLAYING_MEMBER* data = (BPCGEN_UNDERLAYING_MEMBER*)cursor->data;

			if (data->semantic_value->is_basic_type) {
				cpp_type_str = code_cpp_basic_type[data->semantic_value->v_basic_type];
			} else {
				cpp_type_str = data->semantic_value->v_struct_type;
			}

			BPC_CODEGEN_INDENT_PRINT;
			if (data->semantic_value->array_prop.is_array) {
				if (data->semantic_value->array_prop.is_static_array) {
					fprintf(fs_cpp_hdr, "std::array<%s*, %d> %s;",cpp_type_str, data->semantic_value->array_prop.array_len, data->semantic_value->vname);
				} else {
					fprintf(fs_cpp_hdr, "std::vector<%s*> %s;", cpp_type_str, data->semantic_value->vname);
				}
			} else {
				fprintf(fs_cpp_hdr, "%s %s;", cpp_type_str, data->semantic_value->vname);
			}
		}

		// reliable and opcode
		if (is_msg) {
			// reliable
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_cpp_hdr, "virtual bool IsReliable() override;");

			// opcode
			BPC_CODEGEN_INDENT_PRINT;
			fprintf(fs_cpp_hdr, "virtual _OpCode GetOpCode() override;");
		}

		// deserializer
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_cpp_hdr, "virtual bool Deserialize(std::stringstream* data) override;");

		// serializer
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_cpp_hdr, "virtual bool Serialize(std::stringstream* data) override;");

		// class define is over
		BPC_CODEGEN_INDENT_DEC;
		BPC_CODEGEN_INDENT_PRINT;
		fputs("};", fs_cpp_hdr);

	}
	if (BPC_CODEGEN_OUTPUT_CPP_C) {
		;
	}
	if (BPC_CODEGEN_OUTPUT_PROTO) {
		;
	}

	// free operator name gstring
	g_string_free(declare_oper_name, TRUE);
	// free constructed underlaying data
	// we do not need free the variables generated by semantic value
	for (cursor = codegen_member_list; cursor != NULL; cursor = cursor->next) {
		g_free(((BPCGEN_UNDERLAYING_MEMBER*)cursor->data)->c_style_type);
		g_free(cursor->data);
	}
	g_slist_free(codegen_member_list);
}

void _bpcgen_write_preset_code(GSList* namespace_list) {
	// define useful variable
	BPC_CODEGEN_INDENT_INIT;
	GSList* cursor;
	GSList* token_registery = bpcsmtv_token_registery_get_slist();

	// setup template
	if (BPC_CODEGEN_OUTPUT_PYTHON) {
		bpcerr_info(BPCERR_ERROR_SOURCE_CODEGEN, "Python code generation do not support namespace feature.");
		_bpcgen_copy_template(fs_python, u8"snippets/header.py");
	}
	if (BPC_CODEGEN_OUTPUT_CSHARP) {
		// write namespace using
		_bpcgen_copy_template(fs_csharp, u8"snippets/header.cs");

		// then write namespace and helper functions
		// get dot style namespace str first
		GString* strl = g_string_new(NULL);
		GSList* cursor;
		for (cursor = namespace_list; cursor != NULL; cursor = cursor->next) {
			g_string_append(strl, cursor->data);
			if (cursor->next != NULL)
				g_string_append_c(strl, '.');
		}

		// write it
		fprintf(fs_csharp, "namespace %s {\n", strl->str);

		// write template
		_bpcgen_copy_template(fs_csharp, u8"snippets/functions.cs");

		// free namespace name data
		g_string_free(strl, true);
	}
	if (BPC_CODEGEN_OUTPUT_CPP_H) {
		// write include and namespace syntax
		_bpcgen_copy_template(fs_cpp_hdr, u8"snippets/header.hpp");
		// construct namespace bracket
		GSList* cursor;
		for (cursor = namespace_list; cursor != NULL; cursor = cursor->next) {
			fprintf(fs_cpp_hdr, "namespace %s {\n", (gchar*)cursor->data);
		}

		// write opcode enum
		// opcode shoule be written first 
		// because following code need the define 
		// of opcode, otherwise, cpp compiler 
		// will throw a error told it couldn't 
		// find the define of opcode
		bool is_first = true;
		BPC_CODEGEN_INDENT_RESET(fs_cpp_hdr);
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_cpp_hdr, "enum class _OpCode : uint32_t {"); BPC_CODEGEN_INDENT_INC;
		for (cursor = token_registery; cursor != NULL; cursor = cursor->next) {
			BPCSMTV_TOKEN_REGISTERY_ITEM* data = (BPCSMTV_TOKEN_REGISTERY_ITEM*)cursor->data;

			if (data->token_type == BPCSMTV_DEFINED_TOKEN_TYPE_MSG) {
				if (is_first) {
					is_first = false;
				} else {
					fputc(',', fs_cpp_hdr);
				}

				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_cpp_hdr, "%s = %" PRIu32, data->token_name, data->token_extra_props.token_arranged_index);
			}
		}
		// enum opcode is over
		BPC_CODEGEN_INDENT_DEC;
		BPC_CODEGEN_INDENT_PRINT;
		fputc('};', fs_cpp_hdr);

		// write template
		_bpcgen_copy_template(fs_cpp_hdr, u8"snippets/functions.hpp");
	}
	if (BPC_CODEGEN_OUTPUT_CPP_C) {
		if (hpp_reference != NULL)
			fprintf(fs_cpp_src, "#include \"%s\"\n", hpp_reference);

		// construct namespace bracket
		GSList* cursor;
		for (cursor = namespace_list; cursor != NULL; cursor = cursor->next) {
			fprintf(fs_cpp_src, "namespace %s {\n", (gchar*)cursor->data);
		}

		// write template
		_bpcgen_copy_template(fs_cpp_src, u8"snippets/functions.cpp");
	}
	if (BPC_CODEGEN_OUTPUT_PROTO) {
		bpcerr_warning(BPCERR_ERROR_SOURCE_CODEGEN, "Proto generation do not support reliable feature. All reliable attributes will write as annotations.");
	}
}

void _bpcgen_write_tail_code(GSList* namespace_list) {
	if (BPC_CODEGEN_OUTPUT_PYTHON) {
		;
	}
	if (BPC_CODEGEN_OUTPUT_CSHARP) {
		// write namespace right bracket
		fputs("}", fs_csharp);
	}
	if (BPC_CODEGEN_OUTPUT_CPP_H) {
		// construct namespace bracket
		guint count = g_slist_length(namespace_list), i = 0;
		for (i = 0; i < count; ++i) {
			fputc('}', fs_cpp_hdr);
		}
		// break-line
		fputc('\n', fs_cpp_hdr);
	}
	if (BPC_CODEGEN_OUTPUT_CPP_C) {
		// construct namespace bracket
		guint count = g_slist_length(namespace_list), i = 0;
		for (i = 0; i < count; ++i) {
			fputc('}', fs_cpp_src);
		}
		// break-line
		fputc('\n', fs_cpp_src);

		// write template
		_bpcgen_copy_template(fs_cpp_src, u8"snippets/tail.cpp");
	}
	if (BPC_CODEGEN_OUTPUT_PROTO) {
		;
	}
}

void _bpcgen_write_conclusion_code() {
	// define useful variable
	BPC_CODEGEN_INDENT_INIT;
	GSList* cursor;
	GSList* token_registery = bpcsmtv_token_registery_get_slist();

	// ==================== Language Output ====================
	if (BPC_CODEGEN_OUTPUT_PYTHON) {
		BPC_CODEGEN_INDENT_RESET(fs_python);

		// write opcode enum
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_python, "class opcode():"); BPC_CODEGEN_INDENT_INC;
		for (cursor = token_registery; cursor != NULL; cursor = cursor->next) {
			BPCSMTV_TOKEN_REGISTERY_ITEM* data = (BPCSMTV_TOKEN_REGISTERY_ITEM*)cursor->data;

			if (data->token_type == BPCSMTV_DEFINED_TOKEN_TYPE_MSG) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_python, "%s = %d", data->token_name, data->token_extra_props.token_arranged_index);
			}
		}
		// class opcode is over
		BPC_CODEGEN_INDENT_DEC;

		// write uniformed deserialize func
		bool is_first = true;
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_python, "def _uniform_deserialize(ss: io.BytesIO):"); BPC_CODEGEN_INDENT_INC;
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_python, "_opcode = _peek_opcode(ss)");
		for (cursor = token_registery; cursor != NULL; cursor = cursor->next) {
			BPCSMTV_TOKEN_REGISTERY_ITEM* data = (BPCSMTV_TOKEN_REGISTERY_ITEM*)cursor->data;

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
		BPC_CODEGEN_INDENT_RESET(fs_csharp);

		// write opcode enum
		bool is_first = true;
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_csharp, "public enum _OpCode {"); BPC_CODEGEN_INDENT_INC;
		for (cursor = token_registery; cursor != NULL; cursor = cursor->next) {
			BPCSMTV_TOKEN_REGISTERY_ITEM* data = (BPCSMTV_TOKEN_REGISTERY_ITEM*)cursor->data;

			if (data->token_type == BPCSMTV_DEFINED_TOKEN_TYPE_MSG) {
				if (is_first) {
					is_first = false;
				} else {
					fputc(',', fs_csharp);
				}

				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_csharp, "%s = %d", data->token_name, data->token_extra_props.token_arranged_index);
			}
		}
		// enum opcode is over
		BPC_CODEGEN_INDENT_DEC;
		BPC_CODEGEN_INDENT_PRINT;
		fputc('}', fs_csharp);

		// write uniformed deserialize func
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_csharp, "public static partial class _Helper {"); BPC_CODEGEN_INDENT_INC;
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_csharp, "public static _BpcMessage UniformDeserialize(BinaryReader br) {"); BPC_CODEGEN_INDENT_INC;
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_csharp, "var _opcode = br.BpcPeekOpCode();");
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_csharp, "switch (_opcode) {"); BPC_CODEGEN_INDENT_INC;
		for (cursor = token_registery; cursor != NULL; cursor = cursor->next) {
			BPCSMTV_TOKEN_REGISTERY_ITEM* data = (BPCSMTV_TOKEN_REGISTERY_ITEM*)cursor->data;

			if (data->token_type == BPCSMTV_DEFINED_TOKEN_TYPE_MSG) {
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_csharp, "case _OpCode.%s: {", data->token_name);

				// write if body
				BPC_CODEGEN_INDENT_INC;
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_csharp, "var _data = new %s();", data->token_name);
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_csharp, "_data.Deserialize(br);");
				BPC_CODEGEN_INDENT_PRINT;
				fprintf(fs_csharp, "return _data;");

				BPC_CODEGEN_INDENT_DEC;
				BPC_CODEGEN_INDENT_PRINT;
				fputc('}', fs_csharp);
			}
		}
		BPC_CODEGEN_INDENT_DEC;
		BPC_CODEGEN_INDENT_PRINT;
		fputc('}', fs_csharp);

		// default return
		BPC_CODEGEN_INDENT_PRINT;
		fprintf(fs_csharp, "return null;");

		// uniform func is over
		BPC_CODEGEN_INDENT_DEC;
		BPC_CODEGEN_INDENT_PRINT;
		fputc('}', fs_csharp);
		BPC_CODEGEN_INDENT_DEC;
		BPC_CODEGEN_INDENT_PRINT;
		fputc('}', fs_csharp);

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

void _bpcgen_get_underlaying_type(BPCGEN_UNDERLAYING_MEMBER* codegen_member) {
	if (codegen_member->semantic_value->is_basic_type) {
		codegen_member->is_pure_basic_type = true;
		codegen_member->c_style_type = NULL;
		codegen_member->like_basic_type = true;
		codegen_member->underlaying_basic_type = codegen_member->semantic_value->v_basic_type;
	} else {
		// check user defined type
		// token type will never be msg accoring to syntax define
		BPCSMTV_TOKEN_REGISTERY_ITEM* entry = bpcsmtv_token_registery_get(codegen_member->semantic_value->v_struct_type);
		switch (entry->token_type) {
			case BPCSMTV_DEFINED_TOKEN_TYPE_ALIAS:
				codegen_member->is_pure_basic_type = true;
				codegen_member->c_style_type = NULL;
				codegen_member->like_basic_type = true;
				codegen_member->underlaying_basic_type = entry->token_extra_props.token_basic_type;
				break;
			case BPCSMTV_DEFINED_TOKEN_TYPE_ENUM:
				codegen_member->is_pure_basic_type = false;
				codegen_member->c_style_type = g_strdup(codegen_member->semantic_value->v_struct_type);
				codegen_member->like_basic_type = true;
				codegen_member->underlaying_basic_type = entry->token_extra_props.token_basic_type;
				break;
			case BPCSMTV_DEFINED_TOKEN_TYPE_STRUCT:
				codegen_member->is_pure_basic_type = false;
				codegen_member->c_style_type = g_strdup(codegen_member->semantic_value->v_struct_type);
				codegen_member->like_basic_type = false;
				break;
			case BPCSMTV_DEFINED_TOKEN_TYPE_MSG:
			default:
				break;	// skip
		}
	}
}

void _bpcgen_copy_template(FILE* target, const char* u8_template_code_file_path) {
	FILE* fs = bpcfs_open_snippets(u8_template_code_file_path);
	if (fs != NULL) {
		// copy
		char* buffer = g_new0(char, 1024);
		size_t read_counter;

		while ((read_counter = fread(buffer, sizeof(char), 1024, fs)) != 0) {
			fwrite(buffer, sizeof(char), read_counter, target);
		}

		g_free(buffer);
		fclose(fs);
	} else {
		// raise error
		bpcerr_warning(BPCERR_ERROR_SOURCE_CODEGEN, "Fail to open code template file: %s\n\
	Code generation will continue, but generated code may not work.", u8_template_code_file_path);
	}
}
