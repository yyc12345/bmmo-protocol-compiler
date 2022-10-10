#include "main.h"
#include <stdint.h>

typedef struct _BPCDBSET_SMTV_PARSE_NUMBER {
	char* in_string;
	size_t in_start_margin;
	size_t in_end_margin;
	bool out_success;
	gint64 out_num;
}BPCDBSET_SMTV_PARSE_NUMBER;
static const BPCDBSET_SMTV_PARSE_NUMBER dbset_parse_number[] = {
	{ "123", 0u, 0u, true, 123i64 },
	{ "[123]", 1u, 1u, true, 123i64 }
};
static const size_t dblen_parse_number = sizeof(dbset_parse_number) / sizeof(BPCDBSET_SMTV_PARSE_NUMBER);

void bpctest_smtv_parse_number() {

	size_t cursor;
	gint64 result = 0i64;
	bool suc = true;
	BPCDBSET_SMTV_PARSE_NUMBER* data = NULL;
	for (cursor = 0u; cursor < dblen_parse_number; ++cursor) {
		data = dbset_parse_number + cursor;
		suc = bpcsmtv_parse_number(data->in_string, strlen(data->in_string), data->in_start_margin, data->in_end_margin, &result);

		g_assert_true(suc == data->out_success);
		if (suc) {
			g_assert_cmpint(result, ==, data->out_num);
		}
	}

}
