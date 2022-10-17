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

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L1039
size_t _bpcfs_iterator_increase(const gchar* strl, size_t old_pos) {

}

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L968
bool _bpcfs_path_lex_compare(const gchar* lhs, const gchar* rhs) {


}

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L114
size_t _bpcfs_find_separator(const gchar* in_u8path, size_t size) {
	const char* sep = (const char*)memchr(in_u8path, '/', size);
	size_t pos = size;
	if (!!sep) {
		pos = sep - in_u8path;
	}
	return pos;
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
		size_t i = pos + 1;
		for (; i < size; ++i) {
			if (!_bpcfs_is_device_name_char(in_u8path[i]))
				break;
		}

		if (i < size && in_u8path[i] == BPCFS_COLON) {
			pos = i + 1;
			parsing_root_name = false;

			if (pos < size && _bpcfs_is_directory_separator(in_u8path[pos]))
				return pos;
		}
	}
#endif

	if (!parsing_root_name)
		return size;

find_next_separator:
	pos += _bpcfs_find_separator(in_u8path + pos, size - pos);

	return pos;
}

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L811
size_t _bpcfs_find_filename_size(const gchar* in_u8path, size_t root_name_size, size_t end_pos) {
	size_t pos = end_pos;
	while (pos > root_name_size) {
		--pos;

		if (_bpcfs_is_directory_separator(in_u8path[pos])) {
			++pos; // filename starts past the separator
			break;
		}
	}

	return end_pos - pos;
}

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L797
bool _bpcfs_is_root_separator(const gchar* in_u8path, size_t root_dir_pos, size_t pos) {
	/*BOOST_ASSERT_MSG(pos < str.size() && fs::detail::is_directory_separator(str[pos]), "precondition violation");*/

	// root_dir_pos points at the leftmost separator, we need to skip any duplicate separators right of root dir
	while (pos > root_dir_pos && _bpcfs_is_directory_separator(in_u8path[pos - 1]))
		--pos;

	return pos == root_dir_pos;
}

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L434
gchar* _bpcfs_filename(const gchar* u8_path) {
	const size_t size = strlen(u8_path);
	size_t root_dir_pos = _bpcfs_find_root_directory_start(u8_path, size);
	size_t root_name_size = root_dir_pos;
	size_t filename_size, pos;
	if (root_dir_pos < size && _bpcfs_is_directory_separator(u8_path[size - 1]) && is_root_separator(u8_path, root_dir_pos, size - 1)) {
		// Return root directory
		pos = root_dir_pos;
		filename_size = 1u;
	} else if (root_name_size == size) {
		// Return root name
		pos = 0u;
		filename_size = root_name_size;
	} else {
		filename_size = find_filename_size(u8_path, root_name_size, size);
		pos = size - filename_size;
		if (filename_size == 0u && pos > root_name_size && _bpcfs_is_directory_separator(u8_path[pos - 1]) && !_bpcfs_is_root_separator(u8_path, root_dir_pos, pos - 1))
			return g_strdup(BPCFS_DOT_PATH);
	}

	const char* p = u8_path + pos;
	return g_strndup(p, filename_size);
}

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L496
gchar* _bpcfs_extension(gchar* u8_path) {
	gchar* name = _bpcfs_filename(u8_path);
	if (_bpcfs_path_lex_compare(name, BPCFS_DOT_PATH) || _bpcfs_path_lex_compare(name, BPCFS_DOT_DOT_PATH))
		return path();

	gchar* pos = strrchr(name, BPCFS_DOT);
	return pos == NULL ? g_strdup(BPCFS_EMPTY_PATH) : g_strdup(pos);
}

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L312
gchar* bpcfs_replace_extension(const gchar* u8_path, const gchar* u8_ext) {
	//// check param
	//if (u8_path == NULL || u8_ext_with_dot == NULL || u8_ext_with_dot[0] != '.') return g_strdup(u8_path);

	//// get non-dot part
	//gsize ext_size = 0;
	//if (g_str_equal(u8_path, BPCFS_DOT_PATH) || g_str_equal(u8_path, BPCFS_DOT_DOT_PATH)) {
	//	// do not process
	//	return g_strdup(u8_path);
	//}

	//// get dot
	//GString* gstring_path = g_string_new(u8_path);
	//gchar* ext = g_utf8_strrchr(gstring_path->str, gstring_path->len, BPCFS_DOT);
	//if (ext != NULL) {
	//	ext_size = ext - gstring_path->str;
	//}

	//// overwrite it
	//if (ext_size != 0) {
	//	g_string_truncate(gstring_path, gstring_path->len - ext_size);
	//	g_string_append(gstring_path, u8_ext_with_dot);
	//}

	//return g_string_free(gstring_path, false);
}

// Reference: 
// https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L551
// https://stackoverflow.com/questions/27228743/c-function-to-calculate-relative-path
gchar* bpcfs_lexically_relative(const gchar* u8_this, const gchar* u8_base) {
	//// check param
	//if (u8_this == NULL || u8_base == NULL) return g_strdup(u8_this);

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
