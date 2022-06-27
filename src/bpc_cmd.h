#pragma once

#include <glib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct _BPC_CMD_PARSED_ARGS {
	FILE* input_file;
	FILE* out_python_path;
	FILE* out_csharp_path;
	FILE* out_cpp_header_path;
	FILE* out_cpp_source_path;
}BPC_CMD_PARSED_ARGS;

BPC_CMD_PARSED_ARGS* bpccmd_get_parsed_args(int argc, char* argv[]);
void bpccmd_free_parsed_args(BPC_CMD_PARSED_ARGS* struct_args);

BPC_CMD_PARSED_ARGS* _bpccmd_alloc_parsed_args();
void _bpccmd_print_help(GOptionContext* ctx);
void _bpccmd_clean_static_value();
FILE* _bpccmd_open_glib_filename(gchar* glib_filename, bool is_open);