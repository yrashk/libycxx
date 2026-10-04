// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <iterator> in their comment. Only <iterator> is included.
#include <iterator>

#if !defined(__cpp_lib_array_constexpr)
#  error "__cpp_lib_array_constexpr is not defined"
#elif __cpp_lib_array_constexpr != 201811L
#  error "__cpp_lib_array_constexpr != 201811L"
#endif

#if !defined(__cpp_lib_constexpr_iterator)
#  error "__cpp_lib_constexpr_iterator is not defined"
#elif __cpp_lib_constexpr_iterator != 201811L
#  error "__cpp_lib_constexpr_iterator != 201811L"
#endif

#if !defined(__cpp_lib_freestanding_iterator)
#  error "__cpp_lib_freestanding_iterator is not defined"
#elif __cpp_lib_freestanding_iterator != 202306L
#  error "__cpp_lib_freestanding_iterator != 202306L"
#endif

#if !defined(__cpp_lib_make_reverse_iterator)
#  error "__cpp_lib_make_reverse_iterator is not defined"
#elif __cpp_lib_make_reverse_iterator != 201402L
#  error "__cpp_lib_make_reverse_iterator != 201402L"
#endif

#if !defined(__cpp_lib_move_iterator_concept)
#  error "__cpp_lib_move_iterator_concept is not defined"
#elif __cpp_lib_move_iterator_concept != 202207L
#  error "__cpp_lib_move_iterator_concept != 202207L"
#endif

#if !defined(__cpp_lib_null_iterators)
#  error "__cpp_lib_null_iterators is not defined"
#elif __cpp_lib_null_iterators != 201304L
#  error "__cpp_lib_null_iterators != 201304L"
#endif

#if !defined(__cpp_lib_ssize)
#  error "__cpp_lib_ssize is not defined"
#elif __cpp_lib_ssize != 201902L
#  error "__cpp_lib_ssize != 201902L"
#endif
