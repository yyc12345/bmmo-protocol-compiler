#include "main.h"
#include <locale.h>

int main(int argc, char* argv[]) {
    // setup environment
    setlocale(LC_ALL, "C");
    g_test_init(&argc, &argv, NULL);

    // semantic value
    bpctest_semantic_values();
    // fs
    bpctest_fs();
    // cmd
    bpctest_cmd();

    // full compiler test
    // without codegen
    bpctest_project();

    return g_test_run();
}
