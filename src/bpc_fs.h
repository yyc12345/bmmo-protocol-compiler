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

/// <summary>
/// replace file extension lile std::filesystem::path::replace_extension()
/// </summary>
/// <param name="u8path">the path need to be replaced</param>
/// <param name="u8ext">prepared extension string, leading dot is not necessary.</param>
/// <returns></returns>
gchar* bpcfs_replace_extension(const gchar* u8path, const gchar* u8ext);
/// <summary>
/// calculate relative path like std::filesystem::path::lexically_relative()
/// </summary>
/// <param name="u8this">the path need to be evalulated</param>
/// <param name="u8base">provided base path</param>
/// <returns></returns>
gchar* bpcfs_lexically_relative(const gchar* u8this, const gchar* u8base);
