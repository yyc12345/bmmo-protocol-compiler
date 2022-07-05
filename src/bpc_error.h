#pragma once

#include <stdarg.h>

typedef enum _BPCERR_ERROR_SOURCE {
	BPCERR_ERROR_SOURCE_LEX,
	BPCERR_ERROR_SOURCE_PARSER,
	BPCERR_ERROR_SOURCE_SEMANTIC_VALUE,
	BPCERR_ERROR_SOURCE_CODEGEN
}BPCERR_ERROR_SOURCE;

typedef enum _BPCERR_ERROR_TYPE {
	BPCERR_ERROR_TYPE_INFO,
	BPCERR_ERROR_TYPE_WARNING,
	BPCERR_ERROR_TYPE_ERROR
}BPCERR_ERROR_TYPE;

void bpcerr_info(BPCERR_ERROR_SOURCE src, const char* format, ...);
void bpcerr_warning(BPCERR_ERROR_SOURCE src, const char* format, ...);
void bpcerr_error(BPCERR_ERROR_SOURCE src, const char* format, ...);
void bpcerr_panic(BPCERR_ERROR_SOURCE src, const char* format, ...);
void _bpcerr_printf(BPCERR_ERROR_SOURCE src, BPCERR_ERROR_TYPE err_type, const char* format, va_list ap);
void _bpcerr_nuke_process(int rc);
