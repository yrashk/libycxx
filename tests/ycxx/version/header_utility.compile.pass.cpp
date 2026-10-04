// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <utility> in their comment. Only <utility> is included.
#include <utility>

#if !defined(__cpp_lib_as_const)
#  error "__cpp_lib_as_const is not defined"
#elif __cpp_lib_as_const != 201510L
#  error "__cpp_lib_as_const != 201510L"
#endif

#if !defined(__cpp_lib_constexpr_utility)
#  error "__cpp_lib_constexpr_utility is not defined"
#elif __cpp_lib_constexpr_utility != 201811L
#  error "__cpp_lib_constexpr_utility != 201811L"
#endif

#if !defined(__cpp_lib_constrained_equality)
#  error "__cpp_lib_constrained_equality is not defined"
#elif __cpp_lib_constrained_equality != 202411L
#  error "__cpp_lib_constrained_equality != 202411L"
#endif

#if !defined(__cpp_lib_exchange_function)
#  error "__cpp_lib_exchange_function is not defined"
#elif __cpp_lib_exchange_function != 201304L
#  error "__cpp_lib_exchange_function != 201304L"
#endif

#if !defined(__cpp_lib_forward_like)
#  error "__cpp_lib_forward_like is not defined"
#elif __cpp_lib_forward_like != 202207L
#  error "__cpp_lib_forward_like != 202207L"
#endif

#if !defined(__cpp_lib_freestanding_utility)
#  error "__cpp_lib_freestanding_utility is not defined"
#elif __cpp_lib_freestanding_utility != 202306L
#  error "__cpp_lib_freestanding_utility != 202306L"
#endif

#if !defined(__cpp_lib_integer_comparison_functions)
#  error "__cpp_lib_integer_comparison_functions is not defined"
#elif __cpp_lib_integer_comparison_functions != 202002L
#  error "__cpp_lib_integer_comparison_functions != 202002L"
#endif

#if !defined(__cpp_lib_integer_sequence)
#  error "__cpp_lib_integer_sequence is not defined"
#elif __cpp_lib_integer_sequence != 202511L
#  error "__cpp_lib_integer_sequence != 202511L"
#endif

#if !defined(__cpp_lib_observable_checkpoint)
#  error "__cpp_lib_observable_checkpoint is not defined"
#elif __cpp_lib_observable_checkpoint != 202506L
#  error "__cpp_lib_observable_checkpoint != 202506L"
#endif

#if !defined(__cpp_lib_to_underlying)
#  error "__cpp_lib_to_underlying is not defined"
#elif __cpp_lib_to_underlying != 202102L
#  error "__cpp_lib_to_underlying != 202102L"
#endif

#if !defined(__cpp_lib_tuples_by_type)
#  error "__cpp_lib_tuples_by_type is not defined"
#elif __cpp_lib_tuples_by_type != 201304L
#  error "__cpp_lib_tuples_by_type != 201304L"
#endif

#if !defined(__cpp_lib_unreachable)
#  error "__cpp_lib_unreachable is not defined"
#elif __cpp_lib_unreachable != 202202L
#  error "__cpp_lib_unreachable != 202202L"
#endif
