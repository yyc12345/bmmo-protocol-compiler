#include <stdio.h>
#include "y.tab.h"
#include "bpc_cmd.h"

int main(int argc, char* argv[]) {
	BPCCMD_PARSED_ARGS* args = bpccmd_get_parsed_args(argc, argv);
	if (args == NULL) return 0;

	int result = 0;
	if ((result = run_compiler(args, NULL))) {
		printf("Compile Failed.\n");
	} else {
		printf("Compile Done.\n");
	}

	bpccmd_free_parsed_args(args);
	return result;
}
