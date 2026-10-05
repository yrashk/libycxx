// Test-harness shim (see c++config.h), included by testsuite_iterators.h. Its
// contiguous_iterator_wrapper uses libstdc++'s integer-class type
// std::ranges::__detail::__max_diff_type as its difference_type, "to try and break the library
// code" with a difference type other than ptrdiff_t. Which types are integer-class is
// implementation-defined ([iterator.concept.winc]/2-3) and a program-defined class is never one,
// so the harness substitutes libycxx's widest signed-integer-like type: __int128 where the
// compiler has it (an integral type, wider than ptrdiff_t), else long long.
#pragma once
#include <iterator>
namespace std::ranges::__detail {
#ifdef __SIZEOF_INT128__
__extension__ using __max_diff_type = __int128;
__extension__ using __max_size_type = unsigned __int128;
#else
using __max_diff_type = long long;
using __max_size_type = unsigned long long;
#endif
} // namespace std::ranges::__detail
