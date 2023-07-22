#pragma once

#include <glib.h>
#include <stdarg.h>
#include <stdbool.h>

typedef enum _BPCERR_ERROR_SOURCE {
	BPCERR_ERROR_SOURCE_LEX,
	BPCERR_ERROR_SOURCE_PARSER,
	BPCERR_ERROR_SOURCE_SEMANTIC_VALUE,
	BPCERR_ERROR_SOURCE_CODEGEN
}BPCERR_ERROR_SOURCE;

typedef enum _BPCERR_ERROR_CODEGEN_SOURCE {
	BPCERR_ERROR_CODEGEN_SOURCE_NONE,
	BPCERR_ERROR_CODEGEN_SOURCE_PYTHON,
	BPCERR_ERROR_CODEGEN_SOURCE_CSHARP,
	BPCERR_ERROR_CODEGEN_SOURCE_CPP,
	BPCERR_ERROR_CODEGEN_SOURCE_FLATBUFFERS
}BPCERR_ERROR_CODEGEN_SOURCE;

typedef enum _BPCERR_ERROR_TYPE {
	BPCERR_ERROR_TYPE_INFO,
	BPCERR_ERROR_TYPE_WARNING,
	BPCERR_ERROR_TYPE_ERROR
}BPCERR_ERROR_TYPE;

void bpcerr_info(BPCERR_ERROR_SOURCE src, const char* format, ...);
void bpcerr_warning(BPCERR_ERROR_SOURCE src, const char* format, ...);
void bpcerr_error(BPCERR_ERROR_SOURCE src, const char* format, ...);
G_NORETURN void bpcerr_panic(BPCERR_ERROR_SOURCE src, const char* format, ...);

void bpcerr_codegen_info(BPCERR_ERROR_CODEGEN_SOURCE src, const char* format, ...);
void bpcerr_codegen_warning(BPCERR_ERROR_CODEGEN_SOURCE src, const char* format, ...);
void bpcerr_codegen_error(BPCERR_ERROR_CODEGEN_SOURCE src, const char* format, ...);
G_NORETURN void bpcerr_codegen_panic(BPCERR_ERROR_CODEGEN_SOURCE src, const char* format, ...);

