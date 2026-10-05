// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <bitset> in their comment. Only <bitset> is included.
// COUNTERPART: libstdcxx:20_util/bitset/(cons/constexpr_c\+\+23|version).cc
#include <bitset>

#if !defined(__cpp_lib_bitset)
#  error "__cpp_lib_bitset is not defined"
#elif __cpp_lib_bitset != 202306L
#  error "__cpp_lib_bitset != 202306L"
#endif

#if !defined(__cpp_lib_constexpr_bitset)
#  error "__cpp_lib_constexpr_bitset is not defined"
#elif __cpp_lib_constexpr_bitset != 202207L
#  error "__cpp_lib_constexpr_bitset != 202207L"
#endif
