#include "main.h"
#include <locale.h>

int main(int argc, char* argv[]) {
    // setup environment
    setlocale(LC_ALL, "C");
    g_test_init(&argc, &argv, NULL);

    // semantic value
    g_test_add_func("/parse/number", bpctest_smtv_parse_number);

    return g_test_run();
}
