#include "bpc_error.h"
#include <stdbool.h>
#include <stdio.h>

#ifdef G_OS_WIN32
#include <Windows.h>
#else
#include <signal.h>
#endif

static FILE* _bpcerr_error_type_to_stream(BPCERR_ERROR_TYPE err_type) {
	switch (err_type) {
		case BPCERR_ERROR_TYPE_INFO:
			return stdout;
		case BPCERR_ERROR_TYPE_WARNING:
		case BPCERR_ERROR_TYPE_ERROR:
			return stderr;
		default:
			g_assert_not_reached();
	}
}

static void _bpcerr_printf(BPCERR_ERROR_SOURCE src, BPCERR_ERROR_TYPE err_type, const char* format, va_list ap) {
	FILE* stream = _bpcerr_error_type_to_stream(err_type);
	g_assert(stream != NULL);
	bool use_color = g_log_writer_supports_color(fileno(stream));

	// display err type
	switch (err_type) {
		case BPCERR_ERROR_TYPE_INFO:
			if (use_color) fputs("\033[1;32m", stream);	// green
			fputs("[Info ] ", stream);
			break;
		case BPCERR_ERROR_TYPE_WARNING:
			if (use_color) fputs("\033[1;33m", stream);	// yellow
			fputs("[Warn ] ", stream);
			break;
		case BPCERR_ERROR_TYPE_ERROR:
			if (use_color) fputs("\033[1;31m", stream);	// red
			fputs("[Error] ", stream);
			break;
		default:
			g_assert_not_reached();
	}
	if (use_color) fputs("\033[0m", stream);

	// display source
	if (use_color) fputs("\033[1;35m", stream);	// magenta
	switch (src) {
		case BPCERR_ERROR_SOURCE_LEX:
			fputs("[Lexer  ] ", stream);
			break;
		case BPCERR_ERROR_SOURCE_PARSER:
			fputs("[Parser ] ", stream);
			break;
		case BPCERR_ERROR_SOURCE_SEMANTIC_VALUE:
			fputs("[Smt.Val] ", stream);
			break;
		case BPCERR_ERROR_SOURCE_CODEGEN:
			fputs("[CodeGen] ", stream);
			break;
		default:
			g_assert_not_reached();
	}
	if (use_color) fputs("\033[0m", stream);

	// output result
	vfprintf(stream, format, ap);
	fputc('\n', stream);

}

G_NORETURN static void _bpcerr_nuke_process(int rc) {
#ifdef G_OS_WIN32
	ExitProcess(rc);
#else
	(void)rc; // Unused formal parameter
	kill(getpid(), SIGKILL);
#endif
}

void bpcerr_info(BPCERR_ERROR_SOURCE src, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	_bpcerr_printf(src, BPCERR_ERROR_TYPE_INFO, format, ap);
	va_end(ap);
}

void bpcerr_warning(BPCERR_ERROR_SOURCE src, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	_bpcerr_printf(src, BPCERR_ERROR_TYPE_WARNING, format, ap);
	va_end(ap);
}

void bpcerr_error(BPCERR_ERROR_SOURCE src, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	_bpcerr_printf(src, BPCERR_ERROR_TYPE_ERROR, format, ap);
	va_end(ap);
}

G_NORETURN void bpcerr_panic(BPCERR_ERROR_SOURCE src, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	_bpcerr_printf(src, BPCERR_ERROR_TYPE_ERROR, format, ap);
	va_end(ap);

	_bpcerr_nuke_process(1);
}
