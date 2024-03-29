#pragma once

#include <glib.h>

/*
encoding system:
in windows:
glib fs <-> utf8 <-> wchar_t

in linux
glib fs <-> utf8
*/

gchar* bpcenc_glibfs_to_utf8(const gchar* src);
gchar* bpcenc_utf8_to_glibfs(const gchar* src);

#ifdef G_OS_WIN32
#include <Windows.h>

gchar* bpcenc_wchar_to_utf8(const wchar_t* src);
wchar_t* bpcenc_utf8_to_wchar(const gchar* src);

wchar_t* bpcenc_glibfs_to_wchar(const gchar* src);
gchar* bpcenc_wchar_to_glibfs(const wchar_t* src);

#endif
