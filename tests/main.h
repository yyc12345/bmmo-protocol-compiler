#pragma once
#include <glib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "../src/bpc_semantic_values.h"

#define BPCTEST_TEST_DATASET(name, data, datalen, testfunc) {\
size_t cursor;\
gconstpointer pitem = NULL;\
GString* entry_name = g_string_new(NULL);\
for (cursor = 0u; cursor < datalen; ++cursor) {\
	pitem = data + cursor;\
	g_string_printf(entry_name, name "/%" PRIu64, (uint64_t)cursor);\
	g_test_add_data_func(entry_name->str, pitem, testfunc);\
}\
g_string_free(entry_name, true);\
}

void bpctest_semantic_values();
