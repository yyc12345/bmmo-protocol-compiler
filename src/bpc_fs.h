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

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L312
gchar* bpcfs_replace_extension(const gchar* u8path, const gchar* u8ext);
// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L551
gchar* bpcfs_lexically_relative(const gchar* u8this, const gchar* u8base);
