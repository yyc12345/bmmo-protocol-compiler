#include <stdio.h>
#include "y.tab.h"

int main(int argc, char* argv[]) {
	if (argc != 3) {
		printf("[Error] invalid parameter.\n");
		printf("[Error]     Format: bpc bp_file_path code_file_path\n");
		return 1;
	}

	if (run_compiler(argv[1], argv[2])) {
		printf("Compile Done.\n");
	} else {
		printf("Compile Failed.\n");
	}
	return 0;
}
