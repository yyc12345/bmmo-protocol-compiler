cmake_minimum_required(VERSION 3.19)
project(bmmo_protocol_compiler LANGUAGES C)

# check bison and flex
find_package(FLEX REQUIRED)
find_package(BISON 3.6 REQUIRED)

# precompile some files
execute_process(
COMMAND
	"${PROJECT_SOURCE_DIR}/precompile.sh"
WORKING_DIRECTORY
	${PROJECT_SOURCE_DIR}
COMMAND_ERROR_IS_FATAL ANY
)

# find glib and try to link it
find_package(PkgConfig REQUIRED)
pkg_check_modules(Glib REQUIRED glib-2.0>=2.70 IMPORTED_TARGET)

set(CMAKE_C_STANDARD 99)

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
	src/bmmo_protocol_compiler.c
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
target_link_libraries(bmmo_protocol_compiler
PRIVATE
	PkgConfig::Glib
)

target_link_libraries(bpc_test
PRIVATE
	PkgConfig::Glib
)
