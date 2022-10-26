#include "main.h"
#include "../src/bpc_cmd.h"
#include "../src/bpc_fs.h"
#include "../src/bpc_encoding.h"
#include "../src/bpc_semantic_values.h"
#include "../src/y.tab.h"

static void project_test_core(BPCSMTV_DOCUMENT* document) {
	
}

static void project_test_wrapper() {
	// construct a fake args
	BPCCMD_PARSED_ARGS fake_args;

#ifdef G_OS_WIN32
	static const char* test_filename = "examples\\test.bp";
#else
	static const char* test_filename = "examples/test.bp";
#endif

	gchar* glibfs_infile = bpcenc_utf8_to_glibfs(test_filename);
	fake_args.input_file = bpcfs_fopen_glibfs(glibfs_infile, true);
	g_assert(fake_args.input_file != NULL);
	g_free(glibfs_infile);

	fake_args.out_cpp_header_file = fake_args.out_cpp_source_file =
		fake_args.out_csharp_file = fake_args.out_proto_file = fake_args.out_python_file = NULL;
	fake_args.ref_cpp_relative_hdr = NULL;

	// call test
	run_compiler(&fake_args, project_test_core);

	// free fake args
	fclose(fake_args.input_file);
}

void bpctest_project() {
	g_test_add_func("/project", project_test_wrapper);
}
