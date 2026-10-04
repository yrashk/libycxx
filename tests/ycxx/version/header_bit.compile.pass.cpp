// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <bit> in their comment. Only <bit> is included.
#include <bit>

#if !defined(__cpp_lib_bit_cast)
#  error "__cpp_lib_bit_cast is not defined"
#elif __cpp_lib_bit_cast != 201806L
#  error "__cpp_lib_bit_cast != 201806L"
#endif

#if !defined(__cpp_lib_bitops)
#  error "__cpp_lib_bitops is not defined"
#elif __cpp_lib_bitops != 202607L
#  error "__cpp_lib_bitops != 202607L"
#endif

#if !defined(__cpp_lib_byteswap)
#  error "__cpp_lib_byteswap is not defined"
#elif __cpp_lib_byteswap != 202110L
#  error "__cpp_lib_byteswap != 202110L"
#endif

#if !defined(__cpp_lib_endian)
#  error "__cpp_lib_endian is not defined"
#elif __cpp_lib_endian != 201907L
#  error "__cpp_lib_endian != 201907L"
#endif

#if !defined(__cpp_lib_int_pow2)
#  error "__cpp_lib_int_pow2 is not defined"
#elif __cpp_lib_int_pow2 != 202002L
#  error "__cpp_lib_int_pow2 != 202002L"
#endif
