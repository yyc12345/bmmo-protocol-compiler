#pragma once

#include <glib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct _BPCCMD_PARSED_ARGS {
	FILE* input_file;
	FILE* out_python_file;
	FILE* out_csharp_file;
	FILE* out_cpp_header_file;
	FILE* out_cpp_source_file;
	FILE* out_proto_file;
}BPCCMD_PARSED_ARGS;

BPCCMD_PARSED_ARGS* bpccmd_get_parsed_args(int argc, char* argv[]);
void bpccmd_free_parsed_args(BPCCMD_PARSED_ARGS* struct_args);

BPCCMD_PARSED_ARGS* _bpccmd_alloc_parsed_args();
void _bpccmd_print_help(GOptionContext* ctx);
void _bpccmd_clean_static_value();