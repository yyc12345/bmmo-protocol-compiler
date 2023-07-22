#include "bpc_cmd.h"
#include "bpc_ver.h"
#include "bpc_fs.h"
#include "bpc_encoding.h"

static gboolean opt_version = false;
static gboolean opt_help = false;
static gchar* opt_input = NULL;
static gchar* opt_python = NULL;
static gchar* opt_csharp = NULL;
static gchar* opt_cpp_header = NULL;
static gchar* opt_cpp_source = NULL;
static gchar* opt_fbs = NULL;

static GOptionEntry opt_entries[] = {
	{ "help", 'h', 0, G_OPTION_ARG_NONE, &opt_help, "Print help page", NULL },
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &opt_version, "Print compiler version", NULL },
	{ "input", 'i', 0, G_OPTION_ARG_FILENAME, &opt_input, "Input protocol file", "example.bp"},
	{ "python", 'p', 0, G_OPTION_ARG_FILENAME, &opt_python, "Output Python code", "example.py" },
	{ "cs", 'c', 0, G_OPTION_ARG_FILENAME, &opt_csharp, "Output C# code", "example.cs" },
	{ "cpp-header", 'd', 0, G_OPTION_ARG_FILENAME, &opt_cpp_header, "Output C++ header code", "example.hpp"},
	{ "cpp-source", 's', 0, G_OPTION_ARG_FILENAME, &opt_cpp_source, "Output C++ source code", "example.cpp"},
	{ "flatbuffers", 'b', 0, G_OPTION_ARG_FILENAME, &opt_fbs, "Output Flatbuffers code", "example.fbs"},
	G_OPTION_ENTRY_NULL
};

BPCCMD_PARSED_ARGS* bpccmd_get_parsed_args(int argc, char* _argv[]) {
	BPCCMD_PARSED_ARGS* args = NULL;
	GOptionContext* context = NULL;
	gchar** argv;
	GError* error = NULL;

	// get args properly
#ifdef G_OS_WIN32
	argv = g_win32_get_command_line();
#else
	argv = g_strdupv(_argv);
#endif

	// init opt entries
	context = g_option_context_new(NULL);
	g_option_context_set_help_enabled(context, false);
	g_option_context_add_main_entries(context, opt_entries, NULL);

	// parse
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("Error: option parsing failed: %s\n", error->message);
	} else {
		// no error
		if (opt_help) {
			_bpccmd_print_help(context);
		} else if (opt_version) {
			g_print("bmmo-protocol-compiler %s\n", BPCVER_VERSION);
			g_print("Under MIT License. Copyright (c) 2013-2023 BearKidsTeam.\n");
		} else {
			if (opt_input == NULL) {
				g_print("Error: you should specific input file at least.\n\n");
				_bpccmd_print_help(context);
			} else {
				// really no error
				args = _bpccmd_alloc_parsed_args();
			}
		}
	}

	// free necessary variable
	_bpccmd_clean_static_value();
	g_option_context_free(context);
	g_strfreev(argv);

	return args;
}

void bpccmd_free_parsed_args(BPCCMD_PARSED_ARGS* struct_args) {
#define SAFE_CLOSE_FS(fs) if((fs)!=NULL)fclose(fs);
	SAFE_CLOSE_FS(struct_args->input_file);
	SAFE_CLOSE_FS(struct_args->out_python_file);
	SAFE_CLOSE_FS(struct_args->out_csharp_file);
	SAFE_CLOSE_FS(struct_args->out_cpp_header_file);
	SAFE_CLOSE_FS(struct_args->out_cpp_source_file);
	SAFE_CLOSE_FS(struct_args->out_fbs_file);
#undef SAFE_CLOSE_FS
	g_free(struct_args->ref_cpp_relative_hdr);

	g_free(struct_args);
}

gchar* _bpccmd_get_cpp_relative_header(const gchar* glibfs_cpp, const gchar* glibfs_hpp) {
	// no cpp output, write a shit for placeholder
	if (glibfs_cpp == NULL) {
		return NULL;
	}

	if (glibfs_hpp == NULL) {
		// use self name as references header
		gchar* cppname = g_path_get_basename(glibfs_cpp);
		gchar* u8_cppname = bpcenc_glibfs_to_utf8(cppname);
		gchar* u8_hppname = bpcfs_replace_extension(u8_cppname, ".hpp");

		g_free(cppname);
		g_free(u8_cppname);
		return u8_hppname;
	} else {
		// use header name as references header
		// user need to assign find path for this header

		// get hpp file path and cpp located folder path
		gchar* cppfolder = g_path_get_dirname(glibfs_cpp);

		// convert them into absolute path
		// only construct with work directory(wd) when it is not absolute path
		gchar* wd = g_get_current_dir();
		gchar* abscppfolder = g_path_is_absolute(cppfolder) ? g_build_filename(cppfolder, NULL) : g_build_filename(wd, cppfolder, NULL);
		gchar* abshpp = g_path_is_absolute(glibfs_hpp) ? g_build_filename(glibfs_hpp, NULL) : g_build_filename(wd, glibfs_hpp, NULL);

		// calc relative
		gchar* u8_abscppfolder = bpcenc_glibfs_to_utf8(abscppfolder);
		gchar* u8_abshpp = bpcenc_glibfs_to_utf8(abshpp);
		gchar* relative_hpp = bpcfs_lexically_relative(u8_abshpp, u8_abscppfolder);

		// get answer and free data
		gchar* result = NULL;
		if (relative_hpp[0] == '\0') {
			// empty, use absolute hpp path
			result = u8_abshpp;
			g_free(relative_hpp);
		} else {
			result = relative_hpp;
			g_free(u8_abshpp);
		}
		g_free(cppfolder);
		g_free(wd);
		g_free(abscppfolder);
		g_free(abshpp);
		g_free(u8_abscppfolder);

		return result;
	}
}

BPCCMD_PARSED_ARGS* _bpccmd_alloc_parsed_args() {
	BPCCMD_PARSED_ARGS* st = g_new0(BPCCMD_PARSED_ARGS, 1);

	st->input_file = bpcfs_fopen_glibfs(opt_input, true);
	st->out_python_file = bpcfs_fopen_glibfs(opt_python, false);
	st->out_csharp_file = bpcfs_fopen_glibfs(opt_csharp, false);
	st->out_cpp_header_file = bpcfs_fopen_glibfs(opt_cpp_header, false);
	st->out_cpp_source_file = bpcfs_fopen_glibfs(opt_cpp_source, false);
	st->out_fbs_file = bpcfs_fopen_glibfs(opt_fbs, false);

	st->ref_cpp_relative_hdr = _bpccmd_get_cpp_relative_header(opt_cpp_source, opt_cpp_header);

	return st;
}

void _bpccmd_print_help(GOptionContext* ctx) {
	char* helpstr = g_option_context_get_help(ctx, false, NULL);
	g_print("%s", helpstr);
	g_free(helpstr);
}

void _bpccmd_clean_static_value() {
	g_free(opt_input);
	g_free(opt_python);
	g_free(opt_csharp);
	g_free(opt_cpp_header);
	g_free(opt_cpp_source);
	g_free(opt_fbs);

	opt_version = opt_help = false;
	opt_input = opt_python = opt_csharp = opt_cpp_header = opt_cpp_source = opt_fbs = NULL;
}

