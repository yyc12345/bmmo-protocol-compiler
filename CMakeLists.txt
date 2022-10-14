cmake_minimum_required(VERSION 3.10)
project(bmmo_protocol_compiler LANGUAGES C)

# precompile some files
execute_process(
COMMAND
	"${PROJECT_SOURCE_DIR}/precompile.sh"
WORKING_DIRECTORY
	${PROJECT_SOURCE_DIR}
)

# set file collections
set(shared_c 
	src/bpc_cmd.c
	src/bpc_fs.c
	src/bpc_code_gen.c
	src/bpc_semantic_values.c
	src/bpc_encoding.c
	src/bpc_error.c
	src/snippets.c
    src/codegen/codepy.c
    src/codegen/codecs.c
    src/codegen/codehpp.c
    src/codegen/codecpp.c
    src/codegen/codeproto.c
)
set(compiler_c
	src/lex.yy.c
	src/y.tab.c
)
set(tests_c
	tests/main.c
	tests/tests_semantic_values.c
	tests/tests_fs.c
)

# add the executable
add_executable(bmmo_protocol_compiler ${shared_c} ${compiler_c})
add_executable(bpc_test ${shared_c} ${tests_c})

# set include
target_include_directories(bmmo_protocol_compiler
PRIVATE
	"src"
)
target_include_directories(bpc_test
PRIVATE
	"src"
PRIVATE
	"tests"
)

# find glib and try to link it
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(GLIB glib-2.0>=2.6)
include_directories(${GLIB_INCLUDE_DIRS})
link_directories(${GLIB_LIBRARY_DIRS})
add_definitions(${GLIB_CFLAGS_OTHER})
set(CMAKE_C_STANDARD 99)
