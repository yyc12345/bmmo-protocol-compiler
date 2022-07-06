#include "bpc_fs.h"

gchar* bpcfs_vsprintf(const char* format, va_list ap) {
	GString* strl = g_string_new(NULL);
	g_string_vprintf(strl, format, ap);
	return g_string_free(strl, false);
}

FILE* bpcfs_open_snippets(const char* u8_filename) {
	// store program location with glib fs format
	// use static for pernament storage
	static gchar* prgloc = NULL;
	// first, we need get where our program locate
	// first run. get it
	if (prgloc == NULL) {
		int bytes;
		size_t len = 1024;
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
	// and try open it
	FILE* fs = NULL;
	gchar* combined_path = NULL;
	gchar* dir_name = NULL;
	gchar* file_name = NULL;
	if (prgloc != NULL) {
		dir_name = g_path_get_dirname(prgloc);
		file_name = bpcenc_utf8_to_glibfs((char*)u8_filename);
		if (dir_name != NULL && file_name != NULL) {
			combined_path = g_build_filename(dir_name, file_name, NULL);
		}
		if (combined_path != NULL) {
			fs = bpcfs_fopen_glibfs(combined_path, true);
		}
	}
	g_free(combined_path);
	g_free(dir_name);
	g_free(file_name);

	return fs;
}

FILE* bpcfs_fopen_glibfs(const gchar* glibfs_filepath, bool is_open) {
	if (glibfs_filepath == NULL) return NULL;
	FILE* fs = NULL;

#ifdef G_OS_WIN32
	wchar_t* wfilename = bpcenc_glibfs_to_wchar((gchar*)glibfs_filepath);
	if (wfilename == NULL) return NULL;
	fs = _wfopen(wfilename, is_open ? L"r" : L"w");
	g_free(wfilename);
#else
	gchar* utf8_filename = bpcenc_glibfs_to_utf8();
	if (utf8_filename == NULL) return NULL;
	fs = fopen(utf8_filename, is_open ? "r" : "w");
	g_free(utf8_filename);
#endif

	return fs;
}
