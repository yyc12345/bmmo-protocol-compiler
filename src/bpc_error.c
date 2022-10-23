#include "bpc_error.h"
#include <glib.h>
#include <stdbool.h>

#ifdef G_OS_WIN32
#include <Windows.h>
#endif


static void _bpcerr_printf(BPCERR_ERROR_SOURCE src, BPCERR_ERROR_TYPE err_type, const char* format, va_list ap) {
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
			str_type = "";
			break;
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
			str_type = "";
			break;
	}

	g_print("%-10s %-20s %s\n", str_type, str_src, disp->str);

	g_string_free(disp, true);
}

static void _bpcerr_nuke_process(int rc) {
#ifdef G_OS_WIN32
	ExitProcess(rc);
#else
	(void)rc; // Unused formal parameter
	kill(getpid(), SIGKILL);
#endif
}


static bool global_err_blocking = false;
void bpcerr_reset_errblocking() {
	global_err_blocking = false;
}
void bpcerr_set_errblocking() {
	global_err_blocking = true;
}
bool bpcerr_get_errblocking() {
	return global_err_blocking;
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

	bpcerr_set_errblocking();
}

void bpcerr_panic(BPCERR_ERROR_SOURCE src, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	_bpcerr_printf(src, BPCERR_ERROR_TYPE_ERROR, format, ap);
	va_end(ap);

	bpcerr_set_errblocking();
	_bpcerr_nuke_process(1);
}
