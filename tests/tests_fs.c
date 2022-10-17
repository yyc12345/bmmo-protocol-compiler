#include "main.h"
#include "../src/bpc_fs.h"

typedef struct _BPCDB_FS_REPLACE_EXTENSION {
	gchar* in_origin;
	gchar* in_ext;
	gchar* out_expect;
}BPCDB_FS_REPLACE_EXTENSION;
// Ref: https://github.com/boostorg/filesystem/blob/5d4c1caaab65012f939ffa118cc1f36d34198c0b/test/path_test.cpp#L2466
static const BPCDB_FS_REPLACE_EXTENSION dbset_replace_extension[] = {
	{ "", "", "" },
    { "", "a", ".a" },
    { "", "a.", ".a." },
    { "", ".a", ".a" },
    { "", "a.txt", ".a.txt" },
    // see the rationale in html docs for explanation why this works:
    { "", ".txt", ".txt" },

    { "a.txt", "", "a" },
    { "a.txt", "", "a" },
    { "a.txt", ".", "a." },
    { "a.txt", ".tex", "a.tex" },
    { "a.txt", "tex", "a.tex" },
    { "a.", ".tex", "a.tex" },
    { "a.", "tex", "a.tex" },
    { "a", ".txt", "a.txt" },
    { "a", "txt", "a.txt" },
    { "a.b.txt", ".tex", "a.b.tex" },
    { "a.b.txt", "tex", "a.b.tex" },
    { "a/b", ".c", "a/b.c" },
    { "a.txt/b", ".c", "a.txt/b.c" },         // ticket 4702
    { "foo.txt", "exe", "foo.exe" },          // ticket 5118
    { "foo.txt", ".tar.bz2", "foo.tar.bz2" } // ticket 5118
};
static const size_t dblen_replace_extension = sizeof(dbset_replace_extension) / sizeof(BPCDB_FS_REPLACE_EXTENSION);

static void _bpctest_fs_replace_extension(gconstpointer rawdata) {
	const BPCDB_FS_REPLACE_EXTENSION* data = (BPCDB_FS_REPLACE_EXTENSION*)rawdata;
	gchar* result = bpcfs_replace_extension(data->in_origin, data->in_ext);
	g_assert_cmpstr(result, ==, data->out_expect);
	g_free(result);
}
static void bpctest_fs_replace_extension() {
	BPCTEST_TEST_DATASET(
		"/fs/replace_extension", dbset_replace_extension, dblen_replace_extension,
		_bpctest_fs_replace_extension);
}

typedef struct _BPCDB_FS_LEXICALLY_RELATIVE {
	gchar* in_this;
	gchar* in_base;
	gchar* out_relative;
}BPCDB_FS_LEXICALLY_RELATIVE;
// Ref: https://github.com/boostorg/filesystem/blob/4319cf13887aaf5892e0b36c4eab99424277ebaa/test/relative_test.cpp#L27
static const BPCDB_FS_LEXICALLY_RELATIVE dbset_lexically_relative[] = {
	{ "", "", "" },
    { "", "/foo", "" },
    { "/foo", "", "" },
    { "/foo", "/foo", "." },
    { "", "foo", "" },
    { "foo", "", "" },
    { "foo", "foo", "." },

    { "a/b/c", "a", "b/c" },
    { "a//b//c", "a", "b/c" },
    { "a/b/c", "a/b", "c" },
    { "a///b//c", "a//b", "c" },
    { "a/b/c", "a/b/c", "." },
    { "a/b/c", "a/b/c/x", ".." },
    { "a/b/c", "a/b/c/x/y", "../.." },
    { "a/b/c", "a/x", "../b/c" },
    { "a/b/c", "a/b/x", "../c" },
    { "a/b/c", "a/x/y", "../../b/c" },
    { "a/b/c", "a/b/x/y", "../../c" },
    { "a/b/c", "a/b/c/x/y/z", "../../.." },
    { "a/b/c", "a/", "b/c" },
    { "a/b/c", "a/.", "b/c" },
    { "a/b/c", "a/./", "b/c" },
    { "a/b/c", "a/b/..", "" },
    { "a/b/c", "a/b/../", "" },
    { "a/b/c", "a/b/d/..", "c" },
    { "a/b/c", "a/b/d/../", "c" },

    // paths unrelated except first element, and first element is root directory
    { "/a/b/c", "/x", "../a/b/c" },
    { "/a/b/c", "/x/y", "../../a/b/c" },
    { "/a/b/c", "/x/y/z", "../../../a/b/c" },

    // paths unrelated
    { "a/b/c", "x", "" },
    { "a/b/c", "x/y", "" },
    { "a/b/c", "x/y/z", "" },
    { "a/b/c", "/x", "" },
    { "a/b/c", "/x/y", "" },
    { "a/b/c", "/x/y/z", "" },
    { "a/b/c", "/a/b/c", "" },

    // TODO: add some Windows-only test cases that probe presence or absence of
    // drive specifier-and root-directory

    //  Some tests from Jamie Allsop's paper
    { "/a/d", "/a/b/c", "../../d" },
    { "/a/b/c", "/a/d", "../b/c" },
#ifdef G_OS_WIN32
    { "c:\\y", "c:\\x", "../y" },
#else
    { "c:\\y", "c:\\x", "" },
#endif
    { "d:\\y", "c:\\x", "" },

    //  From issue #1976
    { "/foo/new", "/foo/bar", "../new" }
};
static const size_t dblen_lexically_relative = sizeof(dbset_lexically_relative) / sizeof(BPCDB_FS_LEXICALLY_RELATIVE);

static void _bpctest_fs_lexically_relative(gconstpointer rawdata) {
	const BPCDB_FS_LEXICALLY_RELATIVE* data = (BPCDB_FS_LEXICALLY_RELATIVE*)rawdata;
	gchar* result = bpcfs_lexically_relative(data->in_this, data->in_base);
	g_assert_cmpstr(result, ==, data->out_relative);
	g_free(result);
};
static void bpctest_fs_lexically_relative() {
	BPCTEST_TEST_DATASET(
		"/fs/lexically_relative", dbset_lexically_relative, dblen_lexically_relative,
		_bpctest_fs_lexically_relative);
}

void bpctest_fs() {
	bpctest_fs_replace_extension();
	bpctest_fs_lexically_relative();
}
