#include "main.h"
#include <locale.h>

int main(int argc, char* argv[]) {
    // setup environment
    setlocale(LC_ALL, "C");
    g_test_init(&argc, &argv, NULL);

    // semantic value
    g_test_add_func("/parse/int", bpctest_smtv_parse_int);
    g_test_add_func("/parse/uint", bpctest_smtv_parse_uint);

    return g_test_run();
}
