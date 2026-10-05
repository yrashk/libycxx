// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <variant> in their comment. Only <variant> is included.
// COUNTERPART: libstdcxx:20_util/variant/version.cc
#include <variant>

#if !defined(__cpp_lib_constrained_equality)
#  error "__cpp_lib_constrained_equality is not defined"
#elif __cpp_lib_constrained_equality != 202411L
#  error "__cpp_lib_constrained_equality != 202411L"
#endif

#if !defined(__cpp_lib_freestanding_variant)
#  error "__cpp_lib_freestanding_variant is not defined"
#elif __cpp_lib_freestanding_variant != 202311L
#  error "__cpp_lib_freestanding_variant != 202311L"
#endif

#if !defined(__cpp_lib_variant)
#  error "__cpp_lib_variant is not defined"
#elif __cpp_lib_variant != 202306L
#  error "__cpp_lib_variant != 202306L"
#endif
