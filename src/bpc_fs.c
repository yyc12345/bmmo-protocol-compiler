#include "bpc_fs.h"
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

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
	GPtrArray* filenames;
}BPCFS_PATH;
#define BPCFS_PTRARR_PICK(arr, idx) ((char*)((arr)->pdata[(idx)]))
#define BPCFS_STR_IS_EMPTY(str) ((str)[0]=='\0')

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
	data->filenames = g_ptr_array_new_with_free_func(g_free);
	return data;
}
void _bpcfs_destructor_path(BPCFS_PATH* data) {
	if (data == NULL) return;
	g_free(data->root_name);
	g_ptr_array_free(data->filenames, true);
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
	size_t pos = 0u, posinc = 0u;

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
	if ((path->root_directory = _bpcfs_is_directory_separator(u8path[root_name_len]))) {
		++path_cursor;
	}
	
	// get file names
	if (! BPCFS_STR_IS_EMPTY(path_cursor)) {
		size_t sppos = 0u;
		while(_bpcfs_find_separator(path_cursor, &sppos)) {
			// skip successive slash
			if (sppos != 0u) {
				g_ptr_array_add(path->filenames, g_strndup(path_cursor, sppos));
			}
			path_cursor += sppos + 1;
		}
		// process tail file name
		g_ptr_array_add(path->filenames, g_strdup(path_cursor));
	}
	
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
		g_string_append_c(url, BPCFS_PREFERRED_SEPARATOR);
	}
	
	// build filename
	guint c = 0u, arrmax = path->filenames->len;
	for (c = 0u; c < arrmax; ++c) {
		// pick
		gchar* data = BPCFS_PTRARR_PICK(path->filenames, c);
		// build slash and file name
		if (c != 0u) {
			g_string_append_c(url, BPCFS_PREFERRED_SEPARATOR);
		}
		if (data != NULL) {
			g_string_append(url, data);
		}
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
	gchar* oldname = (gchar*)(path->filenames->len == 0u ? NULL :
		g_ptr_array_steal_index(path->filenames, path->filenames->len - 1u));
	GString* newname = g_string_new(oldname);
	size_t namelen = newname->len;
	
	// get dot
	size_t ext_size = 0u;
	if (oldname != NULL) {
		if (g_str_equal(oldname, BPCFS_DOT_PATH) || g_str_equal(oldname, BPCFS_DOT_DOT_PATH)) {
			ext_size = 0u;
		} else {
			char* dotpos = strrchr(oldname, BPCFS_DOT);
			ext_size = (dotpos == NULL ? 0u : namelen - (dotpos - oldname));
		}
	}
	
	// erase
	g_string_truncate(newname, namelen - ext_size);
	
	// write ext
	if (! BPCFS_STR_IS_EMPTY(u8ext)) {
		if (u8ext[0] != BPCFS_DOT) {
			g_string_append_c(newname, BPCFS_DOT);
		}
		g_string_append(newname, u8ext);
	}
	
	// free old one, append new one
	g_free(oldname);
	g_ptr_array_add(path->filenames, g_string_free(newname, false));
	
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

	BPCFS_PATH *thispath = NULL, *basepath = NULL;
	BPCFS_PATH *relpath = NULL;
#define FAKE_DEFER _bpcfs_destructor_path(thispath);\
_bpcfs_destructor_path(basepath);\
_bpcfs_destructor_path(relpath);

	// split path
	thispath = _bpcfs_split_path(u8this);
	basepath = _bpcfs_split_path(u8base);

	// requirements
	if (!g_str_equal(thispath->root_name, basepath->root_name) || (thispath->root_directory ^ basepath->root_directory)) {
		FAKE_DEFER;
		return g_strdup(BPCFS_EMPTY_PATH);
	}

	// mismatch
	guint thislen = thispath->filenames->len, baselen = basepath->filenames->len;
	guint thiscur = 0u, basecur = 0u;
	for (;thiscur < thislen && basecur < baselen && 
			g_str_equal(BPCFS_PTRARR_PICK(thispath->filenames, thiscur), BPCFS_PTRARR_PICK(basepath->filenames, basecur));) {
		++thiscur;
		++basecur;
	}
	// mismatch test
	// if it is fully mismatched, only absolute path can be saved.
	if (thiscur == 0u && basecur == 0u && !thispath->root_directory) {
		FAKE_DEFER;
		return g_strdup(BPCFS_EMPTY_PATH);
	}
	if (thiscur == thislen && basecur == baselen) {
		FAKE_DEFER;
		return g_strdup(BPCFS_DOT_PATH);
	}
	
	// caclulate n
	gint n = 0;
	guint ncur = basecur;
	for(; ncur < baselen; ++ncur) {
		if (g_str_equal(BPCFS_PTRARR_PICK(basepath->filenames, ncur), BPCFS_DOT_DOT_PATH)) {
			--n;
		} else if (!g_str_equal(BPCFS_PTRARR_PICK(basepath->filenames, ncur), BPCFS_DOT_PATH) &&
				! BPCFS_STR_IS_EMPTY(BPCFS_PTRARR_PICK(basepath->filenames, ncur))) {
			++n;
		}
	}
	// considering n
	if (n <0) {
		FAKE_DEFER;
		return g_strdup(BPCFS_EMPTY_PATH);
	}
	if (n == 0 && (thiscur == thislen || BPCFS_STR_IS_EMPTY(BPCFS_PTRARR_PICK(thispath->filenames, thiscur)))) {
		FAKE_DEFER;
		return g_strdup(BPCFS_DOT_PATH);
	}
	
	// construct new one
	// relative path do not need root path
	relpath = _bpcfs_constructor_path();
	relpath->root_name = g_strdup(BPCFS_EMPTY_PATH);
	relpath->root_directory = false;
	// apply n times dot dot path
	while(n-- > 0) {
		g_ptr_array_add(relpath->filenames, g_strdup(BPCFS_DOT_DOT_PATH));
	}
	for(; thiscur < thislen; ++thiscur) {
		g_ptr_array_add(relpath->filenames, g_strdup(BPCFS_PTRARR_PICK(thispath->filenames, thiscur)));
	}
	
	// output data
	gchar* u8rel = _bpcfs_join_path(relpath);
	FAKE_DEFER;
#undef FAKE_DEFER
	return u8rel;
}