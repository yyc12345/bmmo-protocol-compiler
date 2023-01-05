#include "bpc_error.h"
#include <stdbool.h>

#ifdef G_OS_WIN32
#include <Windows.h>
#else
#include <signal.h>
#endif

static void _bpcerr_printf(BPCERR_ERROR_SOURCE src, BPCERR_ERROR_TYPE err_type, const char* format, va_list ap) {
	/*
	GString* disp = g_string_new(NULL);
	g_string_vprintf(disp, format, ap);

	gchar* str_src = NULL, * str_type = NULL;
	switch (err_type) {
		case BPCERR_ERROR_TYPE_INFO:
			str_type = "[Info]";
			break;
		case BPCERR_ERROR_TYPE_WARNING:
			str_type = "[Warning]";
			break;
		case BPCERR_ERROR_TYPE_ERROR:
			str_type = "[Error]";
			break;
		default:
			g_assert_not_reached();
	}

	switch (src) {
		case BPCERR_ERROR_SOURCE_LEX:
			str_src = "[Lex]";
			break;
		case BPCERR_ERROR_SOURCE_PARSER:
			str_src = "[Parser]";
			break;
		case BPCERR_ERROR_SOURCE_SEMANTIC_VALUE:
			str_src = "[Semantic Value]";
			break;
		case BPCERR_ERROR_SOURCE_CODEGEN:
			str_src = "[Code Gen]";
			break;
		default:
			g_assert_not_reached();
	}

	g_print("%-10s %-20s %s\n", str_type, str_src, disp->str);

	g_string_free(disp, true);
	*/

	GLogLevelFlags log_level;
	switch (err_type) {
		case BPCERR_ERROR_TYPE_INFO:
			log_level = G_LOG_LEVEL_MESSAGE;
			break;
		case BPCERR_ERROR_TYPE_WARNING:
			log_level = G_LOG_LEVEL_WARNING;
			break;
		case BPCERR_ERROR_TYPE_ERROR:
			log_level = G_LOG_LEVEL_ERROR;
			break;
		default:
			g_assert_not_reached();
	}

	const gchar* str_src = NULL;
	switch (src) {
		case BPCERR_ERROR_SOURCE_LEX:
			str_src = "[Lexer]";
			break;
		case BPCERR_ERROR_SOURCE_PARSER:
			str_src = "[Parser]";
			break;
		case BPCERR_ERROR_SOURCE_SEMANTIC_VALUE:
			str_src = "[Semantic Value]";
			break;
		case BPCERR_ERROR_SOURCE_CODEGEN:
			str_src = "[Code Gen]";
			break;
		default:
			g_assert_not_reached();
	}

	g_logv(str_src, log_level, format, ap);
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
