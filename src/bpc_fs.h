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

// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L78
bool _bpcfs_is_device_name_char(gchar single);
// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L114
size_t _bpcfs_find_separator(const gchar* in_u8path, size_t size);
// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L831
size_t _bpcfs_find_root_directory_start(const gchar* in_u8path, size_t size);
// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L811
size_t _bpcfs_find_filename_size(const gchar* in_u8path, size_t root_name_size, size_t end_pos);
// Ref: https://github.com/boostorg/filesystem/blob/cffb1d1bbdac53e308d489599138d03fd43f6cbf/include/boost/filesystem/path.hpp#L1347
bool _bpcfs_is_directory_separator(gchar single);
// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L797
bool _bpcfs_is_root_separator(const gchar* in_u8path, size_t root_dir_pos, size_t pos);
// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L434
gchar* _bpcfs_filename(const gchar* u8_path);
// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L496
gchar* _bpcfs_extension(gchar* u8_path);
// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L312
gchar* bpcfs_replace_extension(const gchar* u8_path, const gchar* u8_ext);
// Ref: https://github.com/boostorg/filesystem/blob/9613ccfa4a2c47bbc7059bf61dd52aec11e53893/src/path.cpp#L551
gchar* bpcfs_lexically_relative(const gchar* u8_this, const gchar* u8_base);
