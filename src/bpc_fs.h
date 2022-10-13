#pragma once

#include <glib.h>
#include "bpc_encoding.h"
#include "snippets.h"
#include <stdbool.h>
#include <stdio.h>

#ifdef G_OS_WIN32
#include <Windows.h>
#endif

gchar* bpcfs_vsprintf(const char* format, va_list ap);
void bpcfs_write_snippets(FILE* fs, BPCSNP_EMBEDDED_FILE* snp);
FILE* bpcfs_fopen_glibfs(const gchar* glibfs_filepath, bool is_open);
gchar* bpcfs_replace_ext(gchar* u8_path, const gchar* u8_ext_with_dot);
gchar* bpcfs_simple_relative_path(const gchar* u8_this, const gchar* u8_base);
