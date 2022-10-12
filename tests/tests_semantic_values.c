#include "main.h"

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
	
	{ "-2147483648", 11u, 0u, 0u, true, -2147483648i64 },
	{ "2147483647", 10u, 0u, 0u, true, 2147483647i64 },
	{ "-9223372036854775808", 20u, 0u, 0u, true, -9223372036854775808i64 },
	{ "9223372036854775807", 19u, 0u, 0u, true, 9223372036854775807i64 },
	{ "4294967295", 10u, 0u, 0u, true, 4294967295i64 },
	{ "18446744073709551615", 20u, 0u, 0u, false, 0i64 },

	{ "aaa", 3u, 0u, 0u, false, 0i64 },
	{ "[[\0\t 123\0\t]]", 13u, 2u, 2u, true, 123i64 }
};
static const size_t dblen_parse_int = sizeof(dbset_parse_int) / sizeof(BPCDB_SMTV_PARSE_INT);

static void _bpctest_smtv_parse_int(gconstpointer rawdata) {
	static BPCSMTV_COMPOUND_NUMBER number;
	const BPCDB_SMTV_PARSE_INT* data = (BPCDB_SMTV_PARSE_INT*)rawdata;
	bpcsmtv_parse_number(data->in_string, data->in_len, data->in_start_margin, data->in_end_margin, &number);
	g_assert_true(number.success_int == data->out_success);
	if (number.success_int) {
		g_assert_cmpint(number.num_int, ==, data->out_num);
	}
}
static void bpctest_smtv_parse_int() {
	BPCTEST_TEST_DATASET(
		"/parse/int", dbset_parse_int, dblen_parse_int, 
		_bpctest_smtv_parse_int);
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
	{ "-123", 4u, 0u, 0u, false, 0ui64 },
	
	{ "-2147483648", 11u, 0u, 0u, false, 0ui64 },
	{ "2147483647", 10u, 0u, 0u, true, 2147483647ui64 },
	{ "-9223372036854775808", 20u, 0u, 0u, false, 0ui64 },
	{ "9223372036854775807", 19u, 0u, 0u, true, 9223372036854775807ui64 },
	{ "4294967295", 10u, 0u, 0u, true, 4294967295ui64 },
	{ "18446744073709551615", 20u, 0u, 0u, true, 18446744073709551615ui64 }
};
static const size_t dblen_parse_uint = sizeof(dbset_parse_uint) / sizeof(BPCDB_SMTV_PARSE_UINT);

static void _bpctest_smtv_parse_uint(gconstpointer rawdata) {
	static BPCSMTV_COMPOUND_NUMBER number;
	const BPCDB_SMTV_PARSE_INT* data = (BPCDB_SMTV_PARSE_INT*)rawdata;
	bpcsmtv_parse_number(data->in_string, data->in_len, data->in_start_margin, data->in_end_margin, &number);
	g_assert_true(number.success_uint == data->out_success);
	if (number.success_uint) {
		g_assert_cmpuint(number.num_uint, ==, data->out_num);
	}
}
static void bpctest_smtv_parse_uint() {
	BPCTEST_TEST_DATASET(
		"/parse/uint", dbset_parse_uint, dblen_parse_uint, 
		_bpctest_smtv_parse_uint);
}

void bpctest_semantic_values() {
	// register all test funcs
	bpctest_smtv_parse_int();
	bpctest_smtv_parse_uint();
}
