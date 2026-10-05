// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <memory> in their comment. Only <memory> is included.
// COUNTERPART: libstdcxx:20_util/smartptr.adapt/version.cc
#include <memory>

#if !defined(__cpp_lib_addressof_constexpr)
#  error "__cpp_lib_addressof_constexpr is not defined"
#elif __cpp_lib_addressof_constexpr != 201603L
#  error "__cpp_lib_addressof_constexpr != 201603L"
#endif

#if !defined(__cpp_lib_allocate_at_least)
#  error "__cpp_lib_allocate_at_least is not defined"
#elif __cpp_lib_allocate_at_least != 202302L
#  error "__cpp_lib_allocate_at_least != 202302L"
#endif

#if !defined(__cpp_lib_allocator_traits_is_always_equal)
#  error "__cpp_lib_allocator_traits_is_always_equal is not defined"
#elif __cpp_lib_allocator_traits_is_always_equal != 201411L
#  error "__cpp_lib_allocator_traits_is_always_equal != 201411L"
#endif

#if !defined(__cpp_lib_assume_aligned)
#  error "__cpp_lib_assume_aligned is not defined"
#elif __cpp_lib_assume_aligned != 201811L
#  error "__cpp_lib_assume_aligned != 201811L"
#endif

#if !defined(__cpp_lib_constexpr_dynamic_alloc)
#  error "__cpp_lib_constexpr_dynamic_alloc is not defined"
#elif __cpp_lib_constexpr_dynamic_alloc != 201907L
#  error "__cpp_lib_constexpr_dynamic_alloc != 201907L"
#endif

#if !defined(__cpp_lib_is_sufficiently_aligned)
#  error "__cpp_lib_is_sufficiently_aligned is not defined"
#elif __cpp_lib_is_sufficiently_aligned != 202411L
#  error "__cpp_lib_is_sufficiently_aligned != 202411L"
#endif

#if !defined(__cpp_lib_start_lifetime_as)
#  error "__cpp_lib_start_lifetime_as is not defined"
#elif __cpp_lib_start_lifetime_as != 202207L
#  error "__cpp_lib_start_lifetime_as != 202207L"
#endif

#if !defined(__cpp_lib_to_address)
#  error "__cpp_lib_to_address is not defined"
#elif __cpp_lib_to_address != 201711L
#  error "__cpp_lib_to_address != 201711L"
#endif
