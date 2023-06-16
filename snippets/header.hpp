#pragma once
#include <sstream>
#include <exception>
#include <cstdint>
#include <cstring>
#include <cinttypes>
#include <vector>
#include <type_traits>
#if __cpp_lib_endian || __cpp_lib_byteswap
#include <bit>
#endif
#if _ENABLE_BP_TESTBENCH
#include <functional>
#include <map>
#include <cstddef>
#endif
