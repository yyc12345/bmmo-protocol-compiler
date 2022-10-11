#include "main.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct _BPCDBSET_SMTV_PARSE_INT {
	char* in_string;
	size_t in_start_margin;
	size_t in_end_margin;
	bool out_success;
	gint64 out_num;
}BPCDBSET_SMTV_PARSE_INT;
static const BPCDBSET_SMTV_PARSE_INT dbset_parse_number[] = {
	{ "123", 0u, 0u, true, 123i64 },
	{ "#123", 1u, 0u, true, 123i64 },
	{ "[123]", 1u, 1u, true, 123i64 },
	{ "-123", 0u, 0u, true, -123i64 },
	{ "#-123", 1u, 0u, true, -123i64 },
	{ "[-123]", 1u, 1u, true, -123i64 },

	{ "\0\t 123", 0u, 0u, true, 123i64 },
	{ "123\0\t ", 0u, 0u, true, 123i64 },
	{ "\0\t 123\0\t ", 0u, 0u, true, 123i64 },

	{ "aaa", 0u, 0u, false, 0i64 },
	{ "[[\0\t 123\0\t]]", 1u, 1u, true, 123i64 }
};
static const size_t dblen_parse_number = sizeof(dbset_parse_number) / sizeof(BPCDBSET_SMTV_PARSE_INT);

void bpctest_smtv_parse_int() {

	size_t cursor;
	BPCSMTV_COMPOUND_NUMBER number;
	const BPCDBSET_SMTV_PARSE_INT* data = NULL;
	for (cursor = 0u; cursor < dblen_parse_number; ++cursor) {
		data = dbset_parse_number + cursor;
		bpcsmtv_parse_number(data->in_string, strlen(data->in_string), data->in_start_margin, data->in_end_margin, &number);

		g_assert_true(number.success_int == data->out_success);
		if (number.success_int) {
			g_assert_cmpint(number.num_int, ==, data->out_num);
		}
	}

}

void bpctest_smtv_parse_uint() {

}
