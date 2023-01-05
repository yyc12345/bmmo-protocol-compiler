#include "bpc_encoding.h"

gchar* bpcenc_glibfs_to_utf8(const gchar* src) {
	if (src == NULL) return NULL;
	return g_filename_to_utf8(src, -1, NULL, NULL, NULL);
}

gchar* bpcenc_utf8_to_glibfs(const gchar* src) {
	if (src == NULL) return NULL;
	return g_filename_from_utf8(src, -1, NULL, NULL, NULL);
}


#ifdef G_OS_WIN32

gchar* bpcenc_wchar_to_utf8(const wchar_t* src) {
	if (src == NULL) return NULL;
	/*
	int count, write_result;

	//converter to CHAR
	count = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
	if (count <= 0) return NULL;
	gchar* dest = g_new0(gchar, count);
	write_result = WideCharToMultiByte(CP_UTF8, 0, src, -1, dest, count, NULL, NULL);
	if (write_result <= 0) {
		g_free(dest);
		return NULL;
	}

	return dest;
	*/
	return g_utf16_to_utf8(src, -1, NULL, NULL, NULL);
}

wchar_t* bpcenc_utf8_to_wchar(const gchar* src) {
	if (src == NULL) return NULL;
	/*
	int wcount, write_result;

	// convert to WCHAR
	wcount = MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);
	if (wcount <= 0) return NULL;
	wchar_t* dest = g_new0(wchar_t, wcount);
	write_result = MultiByteToWideChar(CP_UTF8, 0, src, -1, dest, wcount);
	if (write_result <= 0) {
		g_free(dest);
		return NULL;
	}

	return dest;
	*/
	return g_utf8_to_utf16(src, -1, NULL, NULL, NULL);
}


wchar_t* bpcenc_glibfs_to_wchar(const gchar* src) {
	gchar* utf8 = bpcenc_glibfs_to_utf8(src);
	wchar_t* res = bpcenc_utf8_to_wchar(utf8);
	g_free(utf8);
	return res;
}

gchar* bpcenc_wchar_to_glibfs(const wchar_t* src) {
	gchar* utf8 = bpcenc_wchar_to_utf8(src);
	gchar* res = bpcenc_utf8_to_glibfs(utf8);
	g_free(utf8);
	return res;
}

#endif
