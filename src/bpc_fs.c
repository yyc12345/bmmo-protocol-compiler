#include "bpc_fs.h"

#ifdef G_OS_WIN32
#define BPCFS_SPECTATOR u8"/\\"
#define BPCFS_PREFERRED_SPECTATOR u8"\\"
#else
#define BPCFS_SPECTATOR u8"/"
#define BPCFS_PREFERRED_SPECTATOR u8"/"
#endif
#define BPCFS_DOT '.'
#define BPCFS_EMPTY_PATH u8""
#define BPCFS_DOT_PATH u8"."
#define BPCFS_DOT_DOT_PATH u8".."

gchar* bpcfs_vsprintf(const char* format, va_list ap) {
	GString* strl = g_string_new(NULL);
	g_string_vprintf(strl, format, ap);
	return g_string_free(strl, false);
}

void bpcfs_write_snippets(FILE* fs, BPCSNP_EMBEDDED_FILE* snp) {
	fwrite(snp->file, snp->len, sizeof(char), fs);
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

// Reference: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L312
gchar* bpcfs_replace_ext(gchar* u8_path, const gchar* u8_ext_with_dot) {
	// check param
	if (u8_path == NULL || u8_ext_with_dot == NULL || u8_ext_with_dot[0] != '.') return g_strdup(u8_path);

	// get non-dot part
	gsize ext_size = 0;
	if (g_str_equal(u8_path, BPCFS_DOT_PATH) || g_str_equal(u8_path, BPCFS_DOT_DOT_PATH)) {
		// do not process
		return g_strdup(u8_path);
	}

	// get dot
	GString* gstring_path = g_string_new(u8_path);
	gchar* ext = g_utf8_strrchr(gstring_path->str, gstring_path->len, BPCFS_DOT);
	if (ext != NULL) {
		ext_size = ext - gstring_path->str;
	}

	// overwrite it
	if (ext_size != 0) {
		g_string_truncate(gstring_path, gstring_path->len - ext_size);
		g_string_append(gstring_path, u8_ext_with_dot);
	}

	return g_string_free(gstring_path, false);
}

// Reference: 
// https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L551
// https://stackoverflow.com/questions/27228743/c-function-to-calculate-relative-path
gchar* bpcfs_simple_relative_path(const gchar* u8_this, const gchar* u8_base) {
	// check param
	if (u8_this == NULL || u8_base == NULL) return g_strdup(u8_this);

	// split it by spectator
	gchar** splited_this = g_strsplit_set(u8_this, BPCFS_SPECTATOR, -1);
	guint splited_this_len = g_strv_length(splited_this);
	gchar** splited_base = g_strsplit_set(u8_base, BPCFS_SPECTATOR, -1);
	guint splited_base_len = g_strv_length(splited_this);

	// get common part
	guint t, b;
	for (t = b = 0u; t < splited_this_len && b < splited_base_len && g_str_equal(splited_this[t], splited_base[b]);) {
		++t;
		++b;
	}

	// check pre-exit requirements
	if (t == 0 && b == 0) {
		// free allocated splitted list
		g_strfreev(splited_base);
		g_strfreev(splited_this);
		return g_strdup(u8_this);
	}
	if (t == splited_this_len && b == splited_base_len) {
		// free allocated splitted list
		g_strfreev(splited_base);
		g_strfreev(splited_this);
		return g_strdup(BPCFS_DOT_PATH);
	}

	// normal situation process
	gint n = 0;
	for (; b < splited_base_len; ++b) {
		if (g_str_equal(splited_base[b], BPCFS_DOT_DOT_PATH)) {
			--n;
		} else if (!g_str_equal(splited_base[b], BPCFS_EMPTY_PATH) && !g_str_equal(splited_base[b], BPCFS_DOT_PATH)) {
			++n;
		}
	}
	if (n < 0) {
		// free allocated splitted list
		g_strfreev(splited_base);
		g_strfreev(splited_this);
		return g_strdup(u8_this);
	}
	if (n == 0 && (t >= splited_this_len || g_str_equal(splited_this[t], BPCFS_EMPTY_PATH))) {
		// free allocated splitted list
		g_strfreev(splited_base);
		g_strfreev(splited_this);
		return g_strdup(BPCFS_DOT_PATH);
	}

	GSList* result = NULL;
	for (; n > 0; --n) {
		result = g_slist_append(result, g_strdup(BPCFS_DOT_DOT_PATH));
	}
	for(; t < splited_this_len; ++t) {
		result = g_slist_append(result, g_strdup(splited_this[t]));
	}
	GSList* cursor = NULL;
	GString* strl = g_string_new(NULL);
	for (cursor = result; cursor != NULL; cursor = cursor->next) {
		if (cursor != result) {
			g_string_append(strl, BPCFS_PREFERRED_SPECTATOR);
		}
		// use node's string and free it at the same time
		g_string_append(strl, (gchar*)cursor->data);
		g_free(cursor->data);
	}
	g_slist_free(result);

	// free allocated splitted list
	g_strfreev(splited_base);
	g_strfreev(splited_this);
	return g_string_free(strl, false);

}
