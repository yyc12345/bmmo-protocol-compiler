#include "bpc_fs.h"

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
	gchar* utf8_filename = bpcenc_glibfs_to_utf8((gchar*)glibfs_filepath);
	if (utf8_filename == NULL) return NULL;
	fs = fopen(utf8_filename, is_open ? "r" : "w");
	g_free(utf8_filename);
#endif

	return fs;
}

// Reference:
// https://zh.cppreference.com/w/cpp/filesystem/path
// todo: boost document url
typedef struct _BPCFS_PATH {
	gchar* root_name;
	bool root_directory;
	GQueue* filenames;
}BPCFS_PATH;

#define BPCFS_COMMON_PATH (256u)

#define BPCFS_SEPARATOR '/'
#ifdef G_OS_WIN32
#define BPCFS_PREFERRED_SEPARATOR '\\'
#else
#define BPCFS_PREFERRED_SEPARATOR '/'
#endif
#define BPCFS_DOT '.'
#define BPCFS_COLON ':'
#define BPCFS_QUESTIONMARK '?'

#define BPCFS_EMPTY_PATH ""
#define BPCFS_DOT_PATH "."
#define BPCFS_DOT_DOT_PATH ".."

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/windows_tools.hpp#L41
#define _bpcfs_is_letter(c) ((c) >= 'A' && (c) <= 'Z') || \
((c >= 'a' && (c) <= 'z'))

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L78
// https://googleprojectzero.blogspot.com/2016/02/the-definitive-guide-on-win32-to-nt.html
// Device names are:
//
// - PRN
// - AUX
// - NUL
// - CON
// - LPT[1-9]
// - COM[1-9]
// - CONIN$
// - CONOUT$
#define _bpcfs_is_device_char(c) (_bpcfs_is_letter(c) || \
((c) >= '0' && (c) <= '9') || \
(c) == L'$')

// Ref: https://github.com/boostorg/filesystem/blob/cffb1d1bbdac53e308d489599138d03fd43f6cbf/include/boost/filesystem/path.hpp#L1347
#ifdef G_OS_WIN32
#define _bpcfs_is_directory_separator(c) ((c) == BPCFS_SEPARATOR || (c) == BPCFS_PREFERRED_SEPARATOR)
#else
#define _bpcfs_is_directory_separator(c) ((c) == BPCFS_SEPARATOR)
#endif

BPCFS_PATH* _bpcfs_constructor_path() {
	BPCFS_PATH* data = g_new0(BPCFS_PATH, 1);
	data->root_name = NULL;
	data->root_directory = false;
	data->filenames = g_queue_new();
}
void _bpcfs_destructor_path(BPCFS_PATH* data) {
	g_free(data->root_name);
	g_queue_free_full(data->filenames, g_free);
}

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L114
bool _bpcfs_find_separator(const gchar* in_u8path, size_t* out_pos) {
	*out_pos = 0u;
	
	const char* sep = (const char*)strchr(in_u8path, BPCFS_SEPARATOR);
#ifdef G_OS_WIN32
	const char* winsep (const char*)strchr(in_u8path, BPCFS_PREFERRED_SEPARATOR);
	if (winsep != NULL) {
		if (sep == NULL) {
			sep = winsep;
		} else {
			sep = sep < winsep ? sep : winsep;
		}
	}
#endif
	if (sep != NULL) {
		*out_pos = sep - in_u8path;
	}
	return sep != NULL;
}

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L831
size_t _bpcfs_find_root_directory_start(const gchar* in_u8path, size_t size) {
	if (size == 0)
		return 0;

	bool parsing_root_name = false;
	size_t pos = 0;

	// case "//", possibly followed by more characters
	if (_bpcfs_is_directory_separator(in_u8path[0])) {
		if (size >= 2 && _bpcfs_is_directory_separator(in_u8path[1])) {
			if (size == 2) {
				// The whole path is just a pair of separators
				return 2;
			}
#ifdef G_OS_WIN32
			// https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file
			// cases "\\?\" and "\\.\"
			else if (size >= 4 && (in_u8path[2] == BPCFS_QUESTIONMARK || in_u8path[2] == BPCFS_DOT) && _bpcfs_is_directory_separator(in_u8path[3])) {
				parsing_root_name = true;
				pos += 4;
			}
#endif
			else if (_bpcfs_is_directory_separator(in_u8path[2])) {
				// The path starts with three directory separators, which is interpreted as a root directory followed by redundant separators
				return 0;
			} else {
				// case "//net {/}"
				parsing_root_name = true;
				pos += 2;
				goto find_next_separator;
			}
		}
#ifdef G_OS_WIN32
		// https://stackoverflow.com/questions/23041983/path-prefixes-and
		// case "\??\" (NT path prefix)
		else if (size >= 4 && in_u8path[1] == BPCFS_QUESTIONMARK && in_u8path[2] == BPCFS_QUESTIONMARK && _bpcfs_is_directory_separator(in_u8path[3])) {
			parsing_root_name = true;
			pos += 4;
		}
#endif
		else {
			// The path starts with a separator, possibly followed by a non-separator character
			return 0;
		}
	}

#ifdef G_OS_WIN32
	// case "c:" or "prn:"
	// Note: There is ambiguity in a "c:x" path interpretation. It could either mean a file "x" located at the current directory for drive C:,
	//       or an alternative stream "x" of a file "c". Windows API resolve this as the former, and so do we.
	if ((size - pos) >= 2 && _bpcfs_is_letter(in_u8path[pos])) {
		size_t i;
		for (i = pos + 1; i < size; ++i) {
			if (!_bpcfs_is_device_name_char(in_u8path[i])) {
				if (in_u8path[i] == BPCFS_COLON) {
					return pos + 1;
				}
				break;
			}
		}
	}
#endif

	if (!parsing_root_name)
		return 0;

find_next_separator:
	size_t posinc;
	if (_bpcfs_find_separator(in_u8path + pos, &posinc)) {
		pos += posinc;
	}

	return pos;
}

BPCFS_PATH* _bpcfs_split_path(const gchar* u8path) {
	BPCFS_PATH* path = _bpcfs_constructor_path();
	
	// fallback to empty path to ensure return value validation
	if (u8path == NULL) {
		u8path = BPCFS_EMPTY_PATH;
	}
	size_t u8path_len = strlen(u8path);
	
	// get root name
	size_t root_name_len = _bpcfs_find_root_directory_start(u8path, u8path_len);
	path->root_name = g_strndup(u8path, root_name_len);
	
	// get root directory
	const gchar* path_cursor = u8path + root_name_len;
	if (path->root_directory = _bpcfs_is_directory_separator(u8path[root_name_len])) {
		++path_cursor;
	}
	
	// get file names
	size_t sppos = 0u;
	while(_bpc_find_separator(path_cursor, &sppos)) {
		g_queue_push_tail(path->filenames, g_strndup(path_cursor, sppos));
		path_cursor += sppos + 1;
	}
	// process tail file name
	g_queue_push_tail(path->filenames, g_strdup(path_cursor));

	return path;
}

gchar* _bpcfs_join_path(BPCFS_PATH* path) {
	if (path == NULL) return g_strdup(BPCFS_EMPTY_PATH);
	
	// prealloc some space
	GString* url = g_string_sized_new(BPCFS_COMMON_PATH);
	// build root name and directory
	if (path->root_name != NULL) {
		g_string_append(url, path->root_name);
	}
	if (path->root_directory) {
		g_string_append_c(url, BPCFS_PREFERRED_SEPATATOR);
	}
	
	// build filename
	guint c = 0u, qmax = path->filenames->length;
	for (c = 0u; c < qmax; ++c) {
		// pick
		gchar* data = g_queue_pop_head(path->filenames);
		// build slash and file name
		if (c != 0u) {
			g_string_append_c(url, BPCFS_PREFERRED_SEPATATOR);
		}
		if (data != NULL) {
			g_string_append(url, data);
		}
		// push back in circle
		g_queue_push_tail(path->filenames, data);
	}
	
	return g_string_free(url, false);
}

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L312
gchar* bpcfs_replace_extension(const gchar* u8path, const gchar* u8ext) {
	// fallback path to empty
	if (u8path == NULL) u8path = BPCFS_EMPTY_PATH;
	// fallback to empty
	if (u8ext == NULL) u8ext = "";

	// split path
	BPCFS_PATH* path = _bpcfs_split_path(u8path);
	gchar* oldname = g_queue_pop_tail(path->filenames);
	GString* newname = g_string_new(oldname);
	size_t namelen = newname->len;
	
	// get dot
	size_t ext_size = 0;
	if (oldname != NULL) {
		if (g_str_equal(oldname, BPCFS_DOT_PATH) || g_str_equal(oldname, BPCFS_DOT_DOT_PATH) {
			ext_size = 0u;
		}
		char* dotpos = strrchr(oldname, BPCFS_DOT);
		ext_size = namelen - (dotpos == NULL ? 0u : dotpos - oldname);
	}
	
	// erase
	g_string_truncate(newname, namelen - ext_size);
	
	// write ext
	if (u8ext[0] != '\0') {	// not empty
		if (u8ext[0] != BPCFS_DOT) {
			g_string_append_c(newname, BPCFS_DOT);
		}
		g_string_append(newname, u8ext);
	}
	
	// free old one, append new one
	g_free(oldname);
	g_queue_push_tail(path->filenames, g_string_free(newname, false));
	
	// join path and free intermediate struct
	gchar* result = _bpcfs_join_path(path);
	_bpcfs_destructor_path(path);
	return result;
}

// Reference: 
// https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L551
// https://stackoverflow.com/questions/27228743/c-function-to-calculate-relative-path
gchar* bpcfs_lexically_relative(const gchar* u8this, const gchar* u8base) {
	// fallback to empty
	if (u8this == NULL) u8this = BPCFS_EMPTY_PATH;
	if (u8base == NULL) u8base = BPCFS_EMPTY_PATH;

	BPCFS_PATH* thispath = _bpcfs_split_path(u8this);
	BPCFS_PATH* basepath = _bpcfs_split_path(u8base);

	//// split it by spectator
	//gchar** splited_this = g_strsplit_set(u8_this, BPCFS_SPECTATOR, -1);
	//guint splited_this_len = g_strv_length(splited_this);
	//gchar** splited_base = g_strsplit_set(u8_base, BPCFS_SPECTATOR, -1);
	//guint splited_base_len = g_strv_length(splited_this);

	//// get common part
	//guint t, b;
	//for (t = b = 0u; t < splited_this_len && b < splited_base_len && g_str_equal(splited_this[t], splited_base[b]);) {
	//	++t;
	//	++b;
	//}

	//// check pre-exit requirements
	//if (t == 0 && b == 0) {
	//	// free allocated splitted list
	//	g_strfreev(splited_base);
	//	g_strfreev(splited_this);
	//	return g_strdup(u8_this);
	//}
	//if (t == splited_this_len && b == splited_base_len) {
	//	// free allocated splitted list
	//	g_strfreev(splited_base);
	//	g_strfreev(splited_this);
	//	return g_strdup(BPCFS_DOT_PATH);
	//}

	//// normal situation process
	//gint n = 0;
	//for (; b < splited_base_len; ++b) {
	//	if (g_str_equal(splited_base[b], BPCFS_DOT_DOT_PATH)) {
	//		--n;
	//	} else if (!g_str_equal(splited_base[b], BPCFS_EMPTY_PATH) && !g_str_equal(splited_base[b], BPCFS_DOT_PATH)) {
	//		++n;
	//	}
	//}
	//if (n < 0) {
	//	// free allocated splitted list
	//	g_strfreev(splited_base);
	//	g_strfreev(splited_this);
	//	return g_strdup(u8_this);
	//}
	//if (n == 0 && (t >= splited_this_len || g_str_equal(splited_this[t], BPCFS_EMPTY_PATH))) {
	//	// free allocated splitted list
	//	g_strfreev(splited_base);
	//	g_strfreev(splited_this);
	//	return g_strdup(BPCFS_DOT_PATH);
	//}

	//GSList* result = NULL;
	//for (; n > 0; --n) {
	//	result = g_slist_append(result, g_strdup(BPCFS_DOT_DOT_PATH));
	//}
	//for(; t < splited_this_len; ++t) {
	//	result = g_slist_append(result, g_strdup(splited_this[t]));
	//}
	//GSList* cursor = NULL;
	//GString* strl = g_string_new(NULL);
	//for (cursor = result; cursor != NULL; cursor = cursor->next) {
	//	if (cursor != result) {
	//		g_string_append(strl, BPCFS_PREFERRED_SPECTATOR);
	//	}
	//	// use node's string and free it at the same time
	//	g_string_append(strl, (gchar*)cursor->data);
	//	g_free(cursor->data);
	//}
	//g_slist_free(result);

	//// free allocated splitted list
	//g_strfreev(splited_base);
	//g_strfreev(splited_this);
	//return g_string_free(strl, false);

}
