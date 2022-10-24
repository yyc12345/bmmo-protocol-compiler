#include "main.h"
#include "../src/bpc_cmd.h"
#include "../src/bpc_encoding.h"

typedef struct _BPCDB_CMD_GET_RELHPP {
	gchar* in_cpp;
	gchar* in_hpp;
	gchar* out_expected;
}BPCDB_CMD_GET_RELHPP;
static const BPCDB_CMD_GET_RELHPP dbset_get_relhpp[] = {
	{ "test.cpp", NULL, "test.hpp" },

	{ "test.cpp", "test.hpp", "test.hpp" },
	{ "a/test.cpp", "a/test.hpp", "test.hpp" },

	{ "/a/test.cpp", "/a/test.hpp", "test.hpp" },
	{ "/a/b/test.cpp", "/a/test.hpp", "../test.hpp" },
	{ "/a/test.cpp", "/a/b/test.hpp", "b/test.hpp" },

#ifdef G_OS_WIN32
	{ "C:\\test.cpp", "C:\\test.hpp", "test.hpp" },
	{ "C:\\a\\test.cpp", "C:\\a\\test.hpp", "test.hpp" },
#endif

	{ "a/test.cpp", "//ftp.bkt.moe/a/b/test.hpp", "//ftp.bkt.moe/a/b/test.hpp" }

};
static const size_t dblen_get_relhpp = sizeof(dbset_get_relhpp) / sizeof(BPCDB_CMD_GET_RELHPP);

static void _bpctest_cmd_get_relhpp(gconstpointer rawdata) {
	const BPCDB_CMD_GET_RELHPP* data = (BPCDB_CMD_GET_RELHPP*)rawdata;
	gchar* glibfs_cpp = bpcenc_utf8_to_glibfs(data->in_cpp);
	gchar* glibfs_hpp = bpcenc_utf8_to_glibfs(data->in_hpp);

	gchar* result = _bpccmd_get_cpp_relative_header(glibfs_cpp, glibfs_hpp);
	g_assert_cmpstr(result, == , data->out_expected);

	g_free(glibfs_cpp);
	g_free(glibfs_hpp);
	g_free(result);
};
static void bpctest_cmd_get_relhpp() {
	BPCTEST_TEST_DATASET(
		"/cmd/get_cpp_relative_header", dbset_get_relhpp, dblen_get_relhpp,
		_bpctest_cmd_get_relhpp);
}

void bpctest_cmd() {
	bpctest_cmd_get_relhpp();
}
