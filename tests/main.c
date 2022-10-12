#include "main.h"
#include <locale.h>

int main(int argc, char* argv[]) {
    // setup environment
    setlocale(LC_ALL, "C");
    g_test_init(&argc, &argv, NULL);

    // semantic value
    bpctets_semantic_values();

    return g_test_run();
}
