#include "main.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct _BPCDB_SMTV_PARSE_INT {
	char* in_string;
	size_t in_len;
	size_t in_start_margin;
	size_t in_end_margin;
	bool out_success;
	gint64 out_num;
}BPCDB_SMTV_PARSE_INT;
static const BPCDB_SMTV_PARSE_INT dbset_parse_int[] = {
	{ "123", 3u, 0u, 0u, true, 123i64 },
	{ "#123", 4u, 1u, 0u, true, 123i64 },
	{ "[123]", 5u, 1u, 1u, true, 123i64 },
	{ "-123", 4u, 0u, 0u, true, -123i64 },
	{ "#-123", 5u, 1u, 0u, true, -123i64 },
	{ "[-123]", 6u, 1u, 1u, true, -123i64 },

	{ "\0\t 123", 6u, 0u, 0u, true, 123i64 },
	{ "123\0\t ", 6u, 0u, 0u, true, 123i64 },
	{ "\0\t 123\0\t ", 9u, 0u, 0u, true, 123i64 },

	{ "aaa", 3u, 0u, 0u, false, 0i64 },
	{ "[[\0\t 123\0\t]]", 13u, 1u, 1u, true, 123i64 }
};
static const size_t dblen_parse_int = sizeof(dbset_parse_int) / sizeof(BPCDB_SMTV_PARSE_INT);

void bpctest_smtv_parse_int() {
	size_t cursor;
	BPCSMTV_COMPOUND_NUMBER number;
	const BPCDB_SMTV_PARSE_INT* data = NULL;
	for (cursor = 0u; cursor < dblen_parse_int; ++cursor) {
		data = dbset_parse_int + cursor;
		bpcsmtv_parse_number(data->in_string, data->in_len, data->in_start_margin, data->in_end_margin, &number);

		g_assert_true(number.success_int == data->out_success);
		if (number.success_int) {
			g_assert_cmpint(number.num_int, ==, data->out_num);
		}
	}
}

typedef struct _BPCDB_SMTV_PARSE_UINT {
	char* in_string;
	size_t in_len;
	size_t in_start_margin;
	size_t in_end_margin;
	bool out_success;
	guint64 out_num;
}BPCDB_SMTV_PARSE_UINT;
static const BPCDB_SMTV_PARSE_INT dbset_parse_uint[] = {
	{ "123", 3u, 0u, 0u, true, 123ui64 },
	{ "-123", 4u, 0u, 0u, false, 0ui64 }
};
static const size_t dblen_parse_uint = sizeof(dbset_parse_uint) / sizeof(BPCDB_SMTV_PARSE_UINT);

void bpctest_smtv_parse_uint() {
	size_t cursor;
	BPCSMTV_COMPOUND_NUMBER number;
	const BPCDB_SMTV_PARSE_UINT* data = NULL;
	for (cursor = 0u; cursor < dblen_parse_uint; ++cursor) {
		data = dbset_parse_iuint + cursor;
		bpcsmtv_parse_number(data->in_string, data->in_len, data->in_start_margin, data->in_end_margin, &number);

		g_assert_true(number.success_uint == data->out_success);
		if (number.success_uint) {
			g_assert_cmpuint(number.num_uint, ==, data->out_num);
		}
	}
}
