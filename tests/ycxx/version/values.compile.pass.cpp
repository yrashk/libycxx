// [version.syn]: the value of every macro defined by <version> is the integer literal the
// synopsis gives (this draft). Checked for every __cpp_lib_ macro the draft lists, whenever
// the implementation defines it. [version.syn]/4: __cpp_lib_freestanding_operator_new is
// 202306L or 0. [version.syn]/7: a non-hardened implementation should not define the
// __cpp_lib_hardened_ macros (not checked: recommended practice only).
// COUNTERPART: libstdcxx:20_util/function_ref/cons.cc
// COUNTERPART: libstdcxx:21_strings/basic_string/cons/(char|wchar_t)/constexpr.cc
// COUNTERPART: libstdcxx:21_strings/char_traits/requirements/version.cc
// COUNTERPART: libstdcxx:25_algorithms/pstl/feature_test-4.cc libstdcxx:30_threads/barrier/(1|2).cc
// COUNTERPART: libcxx:language.support/support.limits/support.limits.general/[a-z_]+.version.compile.pass.cpp
#include <version>

#if defined(__cpp_lib_adaptor_iterator_pair_constructor) && __cpp_lib_adaptor_iterator_pair_constructor != 202106L
#  error "__cpp_lib_adaptor_iterator_pair_constructor != 202106L"
#endif
#if defined(__cpp_lib_addressof_constexpr) && __cpp_lib_addressof_constexpr != 201603L
#  error "__cpp_lib_addressof_constexpr != 201603L"
#endif
#if defined(__cpp_lib_algorithm_default_value_type) && __cpp_lib_algorithm_default_value_type != 202603L
#  error "__cpp_lib_algorithm_default_value_type != 202603L"
#endif
#if defined(__cpp_lib_algorithm_iterator_requirements) && __cpp_lib_algorithm_iterator_requirements != 202207L
#  error "__cpp_lib_algorithm_iterator_requirements != 202207L"
#endif
#if defined(__cpp_lib_aligned_accessor) && __cpp_lib_aligned_accessor != 202411L
#  error "__cpp_lib_aligned_accessor != 202411L"
#endif
#if defined(__cpp_lib_allocate_at_least) && __cpp_lib_allocate_at_least != 202302L
#  error "__cpp_lib_allocate_at_least != 202302L"
#endif
#if defined(__cpp_lib_allocator_traits_is_always_equal) && __cpp_lib_allocator_traits_is_always_equal != 201411L
#  error "__cpp_lib_allocator_traits_is_always_equal != 201411L"
#endif
#if defined(__cpp_lib_any) && __cpp_lib_any != 201606L
#  error "__cpp_lib_any != 201606L"
#endif
#if defined(__cpp_lib_apply) && __cpp_lib_apply != 202603L
#  error "__cpp_lib_apply != 202603L"
#endif
#if defined(__cpp_lib_array_constexpr) && __cpp_lib_array_constexpr != 201811L
#  error "__cpp_lib_array_constexpr != 201811L"
#endif
#if defined(__cpp_lib_as_const) && __cpp_lib_as_const != 201510L
#  error "__cpp_lib_as_const != 201510L"
#endif
#if defined(__cpp_lib_associative_heterogeneous_erasure) && __cpp_lib_associative_heterogeneous_erasure != 202110L
#  error "__cpp_lib_associative_heterogeneous_erasure != 202110L"
#endif
#if defined(__cpp_lib_associative_heterogeneous_insertion) && __cpp_lib_associative_heterogeneous_insertion != 202306L
#  error "__cpp_lib_associative_heterogeneous_insertion != 202306L"
#endif
#if defined(__cpp_lib_assume_aligned) && __cpp_lib_assume_aligned != 201811L
#  error "__cpp_lib_assume_aligned != 201811L"
#endif
#if defined(__cpp_lib_atomic_flag_test) && __cpp_lib_atomic_flag_test != 201907L
#  error "__cpp_lib_atomic_flag_test != 201907L"
#endif
#if defined(__cpp_lib_atomic_float) && __cpp_lib_atomic_float != 201711L
#  error "__cpp_lib_atomic_float != 201711L"
#endif
#if defined(__cpp_lib_atomic_is_always_lock_free) && __cpp_lib_atomic_is_always_lock_free != 201603L
#  error "__cpp_lib_atomic_is_always_lock_free != 201603L"
#endif
#if defined(__cpp_lib_atomic_lock_free_type_aliases) && __cpp_lib_atomic_lock_free_type_aliases != 201907L
#  error "__cpp_lib_atomic_lock_free_type_aliases != 201907L"
#endif
#if defined(__cpp_lib_atomic_min_max) && __cpp_lib_atomic_min_max != 202506L
#  error "__cpp_lib_atomic_min_max != 202506L"
#endif
#if defined(__cpp_lib_atomic_reductions) && __cpp_lib_atomic_reductions != 202506L
#  error "__cpp_lib_atomic_reductions != 202506L"
#endif
#if defined(__cpp_lib_atomic_ref) && __cpp_lib_atomic_ref != 202603L
#  error "__cpp_lib_atomic_ref != 202603L"
#endif
#if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr != 201711L
#  error "__cpp_lib_atomic_shared_ptr != 201711L"
#endif
#if defined(__cpp_lib_atomic_value_initialization) && __cpp_lib_atomic_value_initialization != 201911L
#  error "__cpp_lib_atomic_value_initialization != 201911L"
#endif
#if defined(__cpp_lib_atomic_wait) && __cpp_lib_atomic_wait != 201907L
#  error "__cpp_lib_atomic_wait != 201907L"
#endif
#if defined(__cpp_lib_barrier) && __cpp_lib_barrier != 202302L
#  error "__cpp_lib_barrier != 202302L"
#endif
#if defined(__cpp_lib_bind_back) && __cpp_lib_bind_back != 202306L
#  error "__cpp_lib_bind_back != 202306L"
#endif
#if defined(__cpp_lib_bind_front) && __cpp_lib_bind_front != 202306L
#  error "__cpp_lib_bind_front != 202306L"
#endif
#if defined(__cpp_lib_bit_cast) && __cpp_lib_bit_cast != 201806L
#  error "__cpp_lib_bit_cast != 201806L"
#endif
#if defined(__cpp_lib_bitops) && __cpp_lib_bitops != 202607L
#  error "__cpp_lib_bitops != 202607L"
#endif
#if defined(__cpp_lib_bitset) && __cpp_lib_bitset != 202306L
#  error "__cpp_lib_bitset != 202306L"
#endif
#if defined(__cpp_lib_bool_constant) && __cpp_lib_bool_constant != 201505L
#  error "__cpp_lib_bool_constant != 201505L"
#endif
#if defined(__cpp_lib_bounded_array_traits) && __cpp_lib_bounded_array_traits != 201902L
#  error "__cpp_lib_bounded_array_traits != 201902L"
#endif
#if defined(__cpp_lib_boyer_moore_searcher) && __cpp_lib_boyer_moore_searcher != 201603L
#  error "__cpp_lib_boyer_moore_searcher != 201603L"
#endif
#if defined(__cpp_lib_byte) && __cpp_lib_byte != 201603L
#  error "__cpp_lib_byte != 201603L"
#endif
#if defined(__cpp_lib_byteswap) && __cpp_lib_byteswap != 202110L
#  error "__cpp_lib_byteswap != 202110L"
#endif
#if defined(__cpp_lib_char8_t) && __cpp_lib_char8_t != 201907L
#  error "__cpp_lib_char8_t != 201907L"
#endif
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono != 202306L
#  error "__cpp_lib_chrono != 202306L"
#endif
#if defined(__cpp_lib_chrono_udls) && __cpp_lib_chrono_udls != 201304L
#  error "__cpp_lib_chrono_udls != 201304L"
#endif
#if defined(__cpp_lib_clamp) && __cpp_lib_clamp != 201603L
#  error "__cpp_lib_clamp != 201603L"
#endif
#if defined(__cpp_lib_common_reference) && __cpp_lib_common_reference != 202302L
#  error "__cpp_lib_common_reference != 202302L"
#endif
#if defined(__cpp_lib_common_reference_wrapper) && __cpp_lib_common_reference_wrapper != 202302L
#  error "__cpp_lib_common_reference_wrapper != 202302L"
#endif
#if defined(__cpp_lib_complex_udls) && __cpp_lib_complex_udls != 201309L
#  error "__cpp_lib_complex_udls != 201309L"
#endif
#if defined(__cpp_lib_concepts) && __cpp_lib_concepts != 202207L
#  error "__cpp_lib_concepts != 202207L"
#endif
#if defined(__cpp_lib_constant_wrapper) && __cpp_lib_constant_wrapper != 202606L
#  error "__cpp_lib_constant_wrapper != 202606L"
#endif
#if defined(__cpp_lib_constexpr_algorithms) && __cpp_lib_constexpr_algorithms != 202306L
#  error "__cpp_lib_constexpr_algorithms != 202306L"
#endif
#if defined(__cpp_lib_constexpr_atomic) && __cpp_lib_constexpr_atomic != 202411L
#  error "__cpp_lib_constexpr_atomic != 202411L"
#endif
#if defined(__cpp_lib_constexpr_bitset) && __cpp_lib_constexpr_bitset != 202207L
#  error "__cpp_lib_constexpr_bitset != 202207L"
#endif
#if defined(__cpp_lib_constexpr_charconv) && __cpp_lib_constexpr_charconv != 202207L
#  error "__cpp_lib_constexpr_charconv != 202207L"
#endif
#if defined(__cpp_lib_constexpr_cmath) && __cpp_lib_constexpr_cmath != 202306L
#  error "__cpp_lib_constexpr_cmath != 202306L"
#endif
#if defined(__cpp_lib_constexpr_complex) && __cpp_lib_constexpr_complex != 202306L
#  error "__cpp_lib_constexpr_complex != 202306L"
#endif
#if defined(__cpp_lib_constexpr_deque) && __cpp_lib_constexpr_deque != 202502L
#  error "__cpp_lib_constexpr_deque != 202502L"
#endif
#if defined(__cpp_lib_constexpr_dynamic_alloc) && __cpp_lib_constexpr_dynamic_alloc != 201907L
#  error "__cpp_lib_constexpr_dynamic_alloc != 201907L"
#endif
#if defined(__cpp_lib_constexpr_exceptions) && __cpp_lib_constexpr_exceptions != 202502L
#  error "__cpp_lib_constexpr_exceptions != 202502L"
#endif
#if defined(__cpp_lib_constexpr_flat_map) && __cpp_lib_constexpr_flat_map != 202502L
#  error "__cpp_lib_constexpr_flat_map != 202502L"
#endif
#if defined(__cpp_lib_constexpr_flat_set) && __cpp_lib_constexpr_flat_set != 202502L
#  error "__cpp_lib_constexpr_flat_set != 202502L"
#endif
#if defined(__cpp_lib_constexpr_format) && __cpp_lib_constexpr_format != 202511L
#  error "__cpp_lib_constexpr_format != 202511L"
#endif
#if defined(__cpp_lib_constexpr_forward_list) && __cpp_lib_constexpr_forward_list != 202502L
#  error "__cpp_lib_constexpr_forward_list != 202502L"
#endif
#if defined(__cpp_lib_constexpr_functional) && __cpp_lib_constexpr_functional != 201907L
#  error "__cpp_lib_constexpr_functional != 201907L"
#endif
#if defined(__cpp_lib_constexpr_inplace_vector) && __cpp_lib_constexpr_inplace_vector != 202502L
#  error "__cpp_lib_constexpr_inplace_vector != 202502L"
#endif
#if defined(__cpp_lib_constexpr_iterator) && __cpp_lib_constexpr_iterator != 201811L
#  error "__cpp_lib_constexpr_iterator != 201811L"
#endif
#if defined(__cpp_lib_constexpr_list) && __cpp_lib_constexpr_list != 202502L
#  error "__cpp_lib_constexpr_list != 202502L"
#endif
#if defined(__cpp_lib_constexpr_map) && __cpp_lib_constexpr_map != 202502L
#  error "__cpp_lib_constexpr_map != 202502L"
#endif
#if defined(__cpp_lib_constexpr_memory) && __cpp_lib_constexpr_memory != 202506L
#  error "__cpp_lib_constexpr_memory != 202506L"
#endif
#if defined(__cpp_lib_constexpr_new) && __cpp_lib_constexpr_new != 202406L
#  error "__cpp_lib_constexpr_new != 202406L"
#endif
#if defined(__cpp_lib_constexpr_numeric) && __cpp_lib_constexpr_numeric != 201911L
#  error "__cpp_lib_constexpr_numeric != 201911L"
#endif
#if defined(__cpp_lib_constexpr_queue) && __cpp_lib_constexpr_queue != 202502L
#  error "__cpp_lib_constexpr_queue != 202502L"
#endif
#if defined(__cpp_lib_constexpr_set) && __cpp_lib_constexpr_set != 202502L
#  error "__cpp_lib_constexpr_set != 202502L"
#endif
#if defined(__cpp_lib_constexpr_stack) && __cpp_lib_constexpr_stack != 202502L
#  error "__cpp_lib_constexpr_stack != 202502L"
#endif
#if defined(__cpp_lib_constexpr_string) && __cpp_lib_constexpr_string != 202511L
#  error "__cpp_lib_constexpr_string != 202511L"
#endif
#if defined(__cpp_lib_constexpr_string_view) && __cpp_lib_constexpr_string_view != 201811L
#  error "__cpp_lib_constexpr_string_view != 201811L"
#endif
#if defined(__cpp_lib_constexpr_tuple) && __cpp_lib_constexpr_tuple != 201811L
#  error "__cpp_lib_constexpr_tuple != 201811L"
#endif
#if defined(__cpp_lib_constexpr_typeinfo) && __cpp_lib_constexpr_typeinfo != 202106L
#  error "__cpp_lib_constexpr_typeinfo != 202106L"
#endif
#if defined(__cpp_lib_constexpr_unordered_map) && __cpp_lib_constexpr_unordered_map != 202502L
#  error "__cpp_lib_constexpr_unordered_map != 202502L"
#endif
#if defined(__cpp_lib_constexpr_unordered_set) && __cpp_lib_constexpr_unordered_set != 202502L
#  error "__cpp_lib_constexpr_unordered_set != 202502L"
#endif
#if defined(__cpp_lib_constexpr_utility) && __cpp_lib_constexpr_utility != 201811L
#  error "__cpp_lib_constexpr_utility != 201811L"
#endif
#if defined(__cpp_lib_constexpr_vector) && __cpp_lib_constexpr_vector != 201907L
#  error "__cpp_lib_constexpr_vector != 201907L"
#endif
#if defined(__cpp_lib_constrained_equality) && __cpp_lib_constrained_equality != 202411L
#  error "__cpp_lib_constrained_equality != 202411L"
#endif
#if defined(__cpp_lib_containers_ranges) && __cpp_lib_containers_ranges != 202202L
#  error "__cpp_lib_containers_ranges != 202202L"
#endif
#if defined(__cpp_lib_contracts) && __cpp_lib_contracts != 202502L
#  error "__cpp_lib_contracts != 202502L"
#endif
#if defined(__cpp_lib_copyable_function) && __cpp_lib_copyable_function != 202306L
#  error "__cpp_lib_copyable_function != 202306L"
#endif
#if defined(__cpp_lib_coroutine) && __cpp_lib_coroutine != 201902L
#  error "__cpp_lib_coroutine != 201902L"
#endif
#if defined(__cpp_lib_counting_scope) && __cpp_lib_counting_scope != 202506L
#  error "__cpp_lib_counting_scope != 202506L"
#endif
#if defined(__cpp_lib_debugging) && __cpp_lib_debugging != 202403L
#  error "__cpp_lib_debugging != 202403L"
#endif
#if defined(__cpp_lib_define_static) && __cpp_lib_define_static != 202506L
#  error "__cpp_lib_define_static != 202506L"
#endif
#if defined(__cpp_lib_destroying_delete) && __cpp_lib_destroying_delete != 201806L
#  error "__cpp_lib_destroying_delete != 201806L"
#endif
#if defined(__cpp_lib_enable_shared_from_this) && __cpp_lib_enable_shared_from_this != 201603L
#  error "__cpp_lib_enable_shared_from_this != 201603L"
#endif
#if defined(__cpp_lib_endian) && __cpp_lib_endian != 201907L
#  error "__cpp_lib_endian != 201907L"
#endif
#if defined(__cpp_lib_erase_if) && __cpp_lib_erase_if != 202002L
#  error "__cpp_lib_erase_if != 202002L"
#endif
#if defined(__cpp_lib_exception_ptr_cast) && __cpp_lib_exception_ptr_cast != 202603L
#  error "__cpp_lib_exception_ptr_cast != 202603L"
#endif
#if defined(__cpp_lib_exchange_function) && __cpp_lib_exchange_function != 201304L
#  error "__cpp_lib_exchange_function != 201304L"
#endif
#if defined(__cpp_lib_execution) && __cpp_lib_execution != 201902L
#  error "__cpp_lib_execution != 201902L"
#endif
#if defined(__cpp_lib_expected) && __cpp_lib_expected != 202606L
#  error "__cpp_lib_expected != 202606L"
#endif
#if defined(__cpp_lib_filesystem) && __cpp_lib_filesystem != 201703L
#  error "__cpp_lib_filesystem != 201703L"
#endif
#if defined(__cpp_lib_flat_map) && __cpp_lib_flat_map != 202511L
#  error "__cpp_lib_flat_map != 202511L"
#endif
#if defined(__cpp_lib_flat_set) && __cpp_lib_flat_set != 202511L
#  error "__cpp_lib_flat_set != 202511L"
#endif
#if defined(__cpp_lib_format) && __cpp_lib_format != 202603L
#  error "__cpp_lib_format != 202603L"
#endif
#if defined(__cpp_lib_format_path) && __cpp_lib_format_path != 202506L
#  error "__cpp_lib_format_path != 202506L"
#endif
#if defined(__cpp_lib_format_ranges) && __cpp_lib_format_ranges != 202207L
#  error "__cpp_lib_format_ranges != 202207L"
#endif
#if defined(__cpp_lib_format_uchar) && __cpp_lib_format_uchar != 202311L
#  error "__cpp_lib_format_uchar != 202311L"
#endif
#if defined(__cpp_lib_formatters) && __cpp_lib_formatters != 202302L
#  error "__cpp_lib_formatters != 202302L"
#endif
#if defined(__cpp_lib_forward_like) && __cpp_lib_forward_like != 202207L
#  error "__cpp_lib_forward_like != 202207L"
#endif
#if defined(__cpp_lib_freestanding_algorithm) && __cpp_lib_freestanding_algorithm != 202502L
#  error "__cpp_lib_freestanding_algorithm != 202502L"
#endif
#if defined(__cpp_lib_freestanding_array) && __cpp_lib_freestanding_array != 202311L
#  error "__cpp_lib_freestanding_array != 202311L"
#endif
#if defined(__cpp_lib_freestanding_char_traits) && __cpp_lib_freestanding_char_traits != 202306L
#  error "__cpp_lib_freestanding_char_traits != 202306L"
#endif
#if defined(__cpp_lib_freestanding_charconv) && __cpp_lib_freestanding_charconv != 202306L
#  error "__cpp_lib_freestanding_charconv != 202306L"
#endif
#if defined(__cpp_lib_freestanding_cstdlib) && __cpp_lib_freestanding_cstdlib != 202306L
#  error "__cpp_lib_freestanding_cstdlib != 202306L"
#endif
#if defined(__cpp_lib_freestanding_cstring) && __cpp_lib_freestanding_cstring != 202311L
#  error "__cpp_lib_freestanding_cstring != 202311L"
#endif
#if defined(__cpp_lib_freestanding_cwchar) && __cpp_lib_freestanding_cwchar != 202306L
#  error "__cpp_lib_freestanding_cwchar != 202306L"
#endif
#if defined(__cpp_lib_freestanding_errc) && __cpp_lib_freestanding_errc != 202306L
#  error "__cpp_lib_freestanding_errc != 202306L"
#endif
#if defined(__cpp_lib_freestanding_execution) && __cpp_lib_freestanding_execution != 202502L
#  error "__cpp_lib_freestanding_execution != 202502L"
#endif
#if defined(__cpp_lib_freestanding_expected) && __cpp_lib_freestanding_expected != 202311L
#  error "__cpp_lib_freestanding_expected != 202311L"
#endif
#if defined(__cpp_lib_freestanding_feature_test_macros) && __cpp_lib_freestanding_feature_test_macros != 202306L
#  error "__cpp_lib_freestanding_feature_test_macros != 202306L"
#endif
#if defined(__cpp_lib_freestanding_functional) && __cpp_lib_freestanding_functional != 202306L
#  error "__cpp_lib_freestanding_functional != 202306L"
#endif
#if defined(__cpp_lib_freestanding_iterator) && __cpp_lib_freestanding_iterator != 202306L
#  error "__cpp_lib_freestanding_iterator != 202306L"
#endif
#if defined(__cpp_lib_freestanding_mdspan) && __cpp_lib_freestanding_mdspan != 202311L
#  error "__cpp_lib_freestanding_mdspan != 202311L"
#endif
#if defined(__cpp_lib_freestanding_memory) && __cpp_lib_freestanding_memory != 202502L
#  error "__cpp_lib_freestanding_memory != 202502L"
#endif
#if defined(__cpp_lib_freestanding_numeric) && __cpp_lib_freestanding_numeric != 202502L
#  error "__cpp_lib_freestanding_numeric != 202502L"
#endif
#if defined(__cpp_lib_freestanding_operator_new) && !(__cpp_lib_freestanding_operator_new == 202306L || __cpp_lib_freestanding_operator_new == 0)
#  error "__cpp_lib_freestanding_operator_new: neither 202306L nor 0"
#endif
#if defined(__cpp_lib_freestanding_optional) && __cpp_lib_freestanding_optional != 202506L
#  error "__cpp_lib_freestanding_optional != 202506L"
#endif
#if defined(__cpp_lib_freestanding_random) && __cpp_lib_freestanding_random != 202502L
#  error "__cpp_lib_freestanding_random != 202502L"
#endif
#if defined(__cpp_lib_freestanding_ranges) && __cpp_lib_freestanding_ranges != 202306L
#  error "__cpp_lib_freestanding_ranges != 202306L"
#endif
#if defined(__cpp_lib_freestanding_ratio) && __cpp_lib_freestanding_ratio != 202306L
#  error "__cpp_lib_freestanding_ratio != 202306L"
#endif
#if defined(__cpp_lib_freestanding_string_view) && __cpp_lib_freestanding_string_view != 202311L
#  error "__cpp_lib_freestanding_string_view != 202311L"
#endif
#if defined(__cpp_lib_freestanding_tuple) && __cpp_lib_freestanding_tuple != 202306L
#  error "__cpp_lib_freestanding_tuple != 202306L"
#endif
#if defined(__cpp_lib_freestanding_utility) && __cpp_lib_freestanding_utility != 202306L
#  error "__cpp_lib_freestanding_utility != 202306L"
#endif
#if defined(__cpp_lib_freestanding_variant) && __cpp_lib_freestanding_variant != 202311L
#  error "__cpp_lib_freestanding_variant != 202311L"
#endif
#if defined(__cpp_lib_fstream_native_handle) && __cpp_lib_fstream_native_handle != 202306L
#  error "__cpp_lib_fstream_native_handle != 202306L"
#endif
#if defined(__cpp_lib_function_ref) && __cpp_lib_function_ref != 202604L
#  error "__cpp_lib_function_ref != 202604L"
#endif
#if defined(__cpp_lib_gcd_lcm) && __cpp_lib_gcd_lcm != 201606L
#  error "__cpp_lib_gcd_lcm != 201606L"
#endif
#if defined(__cpp_lib_generator) && __cpp_lib_generator != 202207L
#  error "__cpp_lib_generator != 202207L"
#endif
#if defined(__cpp_lib_generic_associative_lookup) && __cpp_lib_generic_associative_lookup != 201304L
#  error "__cpp_lib_generic_associative_lookup != 201304L"
#endif
#if defined(__cpp_lib_generic_unordered_lookup) && __cpp_lib_generic_unordered_lookup != 201811L
#  error "__cpp_lib_generic_unordered_lookup != 201811L"
#endif
#if defined(__cpp_lib_hardened_array) && __cpp_lib_hardened_array != 202502L
#  error "__cpp_lib_hardened_array != 202502L"
#endif
#if defined(__cpp_lib_hardened_basic_stacktrace) && __cpp_lib_hardened_basic_stacktrace != 202506L
#  error "__cpp_lib_hardened_basic_stacktrace != 202506L"
#endif
#if defined(__cpp_lib_hardened_basic_string) && __cpp_lib_hardened_basic_string != 202502L
#  error "__cpp_lib_hardened_basic_string != 202502L"
#endif
#if defined(__cpp_lib_hardened_basic_string_view) && __cpp_lib_hardened_basic_string_view != 202502L
#  error "__cpp_lib_hardened_basic_string_view != 202502L"
#endif
#if defined(__cpp_lib_hardened_bitset) && __cpp_lib_hardened_bitset != 202502L
#  error "__cpp_lib_hardened_bitset != 202502L"
#endif
#if defined(__cpp_lib_hardened_common_iterator) && __cpp_lib_hardened_common_iterator != 202506L
#  error "__cpp_lib_hardened_common_iterator != 202506L"
#endif
#if defined(__cpp_lib_hardened_counted_iterator) && __cpp_lib_hardened_counted_iterator != 202506L
#  error "__cpp_lib_hardened_counted_iterator != 202506L"
#endif
#if defined(__cpp_lib_hardened_deque) && __cpp_lib_hardened_deque != 202502L
#  error "__cpp_lib_hardened_deque != 202502L"
#endif
#if defined(__cpp_lib_hardened_expected) && __cpp_lib_hardened_expected != 202502L
#  error "__cpp_lib_hardened_expected != 202502L"
#endif
#if defined(__cpp_lib_hardened_forward_list) && __cpp_lib_hardened_forward_list != 202502L
#  error "__cpp_lib_hardened_forward_list != 202502L"
#endif
#if defined(__cpp_lib_hardened_inplace_vector) && __cpp_lib_hardened_inplace_vector != 202502L
#  error "__cpp_lib_hardened_inplace_vector != 202502L"
#endif
#if defined(__cpp_lib_hardened_list) && __cpp_lib_hardened_list != 202502L
#  error "__cpp_lib_hardened_list != 202502L"
#endif
#if defined(__cpp_lib_hardened_mdspan) && __cpp_lib_hardened_mdspan != 202502L
#  error "__cpp_lib_hardened_mdspan != 202502L"
#endif
#if defined(__cpp_lib_hardened_optional) && __cpp_lib_hardened_optional != 202502L
#  error "__cpp_lib_hardened_optional != 202502L"
#endif
#if defined(__cpp_lib_hardened_shared_ptr_array) && __cpp_lib_hardened_shared_ptr_array != 202506L
#  error "__cpp_lib_hardened_shared_ptr_array != 202506L"
#endif
#if defined(__cpp_lib_hardened_span) && __cpp_lib_hardened_span != 202502L
#  error "__cpp_lib_hardened_span != 202502L"
#endif
#if defined(__cpp_lib_hardened_valarray) && __cpp_lib_hardened_valarray != 202502L
#  error "__cpp_lib_hardened_valarray != 202502L"
#endif
#if defined(__cpp_lib_hardened_vector) && __cpp_lib_hardened_vector != 202502L
#  error "__cpp_lib_hardened_vector != 202502L"
#endif
#if defined(__cpp_lib_hardened_view_interface) && __cpp_lib_hardened_view_interface != 202506L
#  error "__cpp_lib_hardened_view_interface != 202506L"
#endif
#if defined(__cpp_lib_hardware_interference_size) && __cpp_lib_hardware_interference_size != 201703L
#  error "__cpp_lib_hardware_interference_size != 201703L"
#endif
#if defined(__cpp_lib_has_unique_object_representations) && __cpp_lib_has_unique_object_representations != 201606L
#  error "__cpp_lib_has_unique_object_representations != 201606L"
#endif
#if defined(__cpp_lib_hazard_pointer) && __cpp_lib_hazard_pointer != 202606L
#  error "__cpp_lib_hazard_pointer != 202606L"
#endif
#if defined(__cpp_lib_hive) && __cpp_lib_hive != 202502L
#  error "__cpp_lib_hive != 202502L"
#endif
#if defined(__cpp_lib_hypot) && __cpp_lib_hypot != 201603L
#  error "__cpp_lib_hypot != 201603L"
#endif
#if defined(__cpp_lib_incomplete_container_elements) && __cpp_lib_incomplete_container_elements != 201505L
#  error "__cpp_lib_incomplete_container_elements != 201505L"
#endif
#if defined(__cpp_lib_indirect) && __cpp_lib_indirect != 202502L
#  error "__cpp_lib_indirect != 202502L"
#endif
#if defined(__cpp_lib_initializer_list) && __cpp_lib_initializer_list != 202511L
#  error "__cpp_lib_initializer_list != 202511L"
#endif
#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector != 202603L
#  error "__cpp_lib_inplace_vector != 202603L"
#endif
#if defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 != 202002L
#  error "__cpp_lib_int_pow2 != 202002L"
#endif
#if defined(__cpp_lib_integer_comparison_functions) && __cpp_lib_integer_comparison_functions != 202002L
#  error "__cpp_lib_integer_comparison_functions != 202002L"
#endif
#if defined(__cpp_lib_integer_sequence) && __cpp_lib_integer_sequence != 202511L
#  error "__cpp_lib_integer_sequence != 202511L"
#endif
#if defined(__cpp_lib_integral_constant_callable) && __cpp_lib_integral_constant_callable != 201304L
#  error "__cpp_lib_integral_constant_callable != 201304L"
#endif
#if defined(__cpp_lib_interpolate) && __cpp_lib_interpolate != 201902L
#  error "__cpp_lib_interpolate != 201902L"
#endif
#if defined(__cpp_lib_invoke) && __cpp_lib_invoke != 201411L
#  error "__cpp_lib_invoke != 201411L"
#endif
#if defined(__cpp_lib_invoke_r) && __cpp_lib_invoke_r != 202106L
#  error "__cpp_lib_invoke_r != 202106L"
#endif
#if defined(__cpp_lib_ios_noreplace) && __cpp_lib_ios_noreplace != 202207L
#  error "__cpp_lib_ios_noreplace != 202207L"
#endif
#if defined(__cpp_lib_is_aggregate) && __cpp_lib_is_aggregate != 201703L
#  error "__cpp_lib_is_aggregate != 201703L"
#endif
#if defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated != 201811L
#  error "__cpp_lib_is_constant_evaluated != 201811L"
#endif
#if defined(__cpp_lib_is_final) && __cpp_lib_is_final != 201402L
#  error "__cpp_lib_is_final != 201402L"
#endif
#if defined(__cpp_lib_is_implicit_lifetime) && __cpp_lib_is_implicit_lifetime != 202302L
#  error "__cpp_lib_is_implicit_lifetime != 202302L"
#endif
#if defined(__cpp_lib_is_invocable) && __cpp_lib_is_invocable != 201703L
#  error "__cpp_lib_is_invocable != 201703L"
#endif
#if defined(__cpp_lib_is_layout_compatible) && __cpp_lib_is_layout_compatible != 201907L
#  error "__cpp_lib_is_layout_compatible != 201907L"
#endif
#if defined(__cpp_lib_is_nothrow_convertible) && __cpp_lib_is_nothrow_convertible != 201806L
#  error "__cpp_lib_is_nothrow_convertible != 201806L"
#endif
#if defined(__cpp_lib_is_null_pointer) && __cpp_lib_is_null_pointer != 201309L
#  error "__cpp_lib_is_null_pointer != 201309L"
#endif
#if defined(__cpp_lib_is_pointer_interconvertible) && __cpp_lib_is_pointer_interconvertible != 201907L
#  error "__cpp_lib_is_pointer_interconvertible != 201907L"
#endif
#if defined(__cpp_lib_is_scoped_enum) && __cpp_lib_is_scoped_enum != 202011L
#  error "__cpp_lib_is_scoped_enum != 202011L"
#endif
#if defined(__cpp_lib_is_structural) && __cpp_lib_is_structural != 202603L
#  error "__cpp_lib_is_structural != 202603L"
#endif
#if defined(__cpp_lib_is_sufficiently_aligned) && __cpp_lib_is_sufficiently_aligned != 202411L
#  error "__cpp_lib_is_sufficiently_aligned != 202411L"
#endif
#if defined(__cpp_lib_is_swappable) && __cpp_lib_is_swappable != 201603L
#  error "__cpp_lib_is_swappable != 201603L"
#endif
#if defined(__cpp_lib_is_virtual_base_of) && __cpp_lib_is_virtual_base_of != 202406L
#  error "__cpp_lib_is_virtual_base_of != 202406L"
#endif
#if defined(__cpp_lib_is_within_lifetime) && __cpp_lib_is_within_lifetime != 202603L
#  error "__cpp_lib_is_within_lifetime != 202603L"
#endif
#if defined(__cpp_lib_jthread) && __cpp_lib_jthread != 201911L
#  error "__cpp_lib_jthread != 201911L"
#endif
#if defined(__cpp_lib_latch) && __cpp_lib_latch != 201907L
#  error "__cpp_lib_latch != 201907L"
#endif
#if defined(__cpp_lib_launder) && __cpp_lib_launder != 201606L
#  error "__cpp_lib_launder != 201606L"
#endif
#if defined(__cpp_lib_linalg) && __cpp_lib_linalg != 202511L
#  error "__cpp_lib_linalg != 202511L"
#endif
#if defined(__cpp_lib_list_remove_return_type) && __cpp_lib_list_remove_return_type != 201806L
#  error "__cpp_lib_list_remove_return_type != 201806L"
#endif
#if defined(__cpp_lib_logical_traits) && __cpp_lib_logical_traits != 201510L
#  error "__cpp_lib_logical_traits != 201510L"
#endif
#if defined(__cpp_lib_make_from_tuple) && __cpp_lib_make_from_tuple != 201606L
#  error "__cpp_lib_make_from_tuple != 201606L"
#endif
#if defined(__cpp_lib_make_reverse_iterator) && __cpp_lib_make_reverse_iterator != 201402L
#  error "__cpp_lib_make_reverse_iterator != 201402L"
#endif
#if defined(__cpp_lib_make_unique) && __cpp_lib_make_unique != 201304L
#  error "__cpp_lib_make_unique != 201304L"
#endif
#if defined(__cpp_lib_map_lookup) && __cpp_lib_map_lookup != 202606L
#  error "__cpp_lib_map_lookup != 202606L"
#endif
#if defined(__cpp_lib_map_try_emplace) && __cpp_lib_map_try_emplace != 201411L
#  error "__cpp_lib_map_try_emplace != 201411L"
#endif
#if defined(__cpp_lib_math_constants) && __cpp_lib_math_constants != 201907L
#  error "__cpp_lib_math_constants != 201907L"
#endif
#if defined(__cpp_lib_math_special_functions) && __cpp_lib_math_special_functions != 201603L
#  error "__cpp_lib_math_special_functions != 201603L"
#endif
#if defined(__cpp_lib_mdspan) && __cpp_lib_mdspan != 202406L
#  error "__cpp_lib_mdspan != 202406L"
#endif
#if defined(__cpp_lib_mdspan_copy) && __cpp_lib_mdspan_copy != 202606L
#  error "__cpp_lib_mdspan_copy != 202606L"
#endif
#if defined(__cpp_lib_memory_resource) && __cpp_lib_memory_resource != 201603L
#  error "__cpp_lib_memory_resource != 201603L"
#endif
#if defined(__cpp_lib_modules) && __cpp_lib_modules != 202207L
#  error "__cpp_lib_modules != 202207L"
#endif
#if defined(__cpp_lib_move_iterator_concept) && __cpp_lib_move_iterator_concept != 202207L
#  error "__cpp_lib_move_iterator_concept != 202207L"
#endif
#if defined(__cpp_lib_move_only_function) && __cpp_lib_move_only_function != 202110L
#  error "__cpp_lib_move_only_function != 202110L"
#endif
#if defined(__cpp_lib_node_extract) && __cpp_lib_node_extract != 201606L
#  error "__cpp_lib_node_extract != 201606L"
#endif
#if defined(__cpp_lib_nonmember_container_access) && __cpp_lib_nonmember_container_access != 201411L
#  error "__cpp_lib_nonmember_container_access != 201411L"
#endif
#if defined(__cpp_lib_not_fn) && __cpp_lib_not_fn != 202306L
#  error "__cpp_lib_not_fn != 202306L"
#endif
#if defined(__cpp_lib_null_iterators) && __cpp_lib_null_iterators != 201304L
#  error "__cpp_lib_null_iterators != 201304L"
#endif
#if defined(__cpp_lib_observable_checkpoint) && __cpp_lib_observable_checkpoint != 202506L
#  error "__cpp_lib_observable_checkpoint != 202506L"
#endif
#if defined(__cpp_lib_optional) && __cpp_lib_optional != 202506L
#  error "__cpp_lib_optional != 202506L"
#endif
#if defined(__cpp_lib_optional_range_support) && __cpp_lib_optional_range_support != 202406L
#  error "__cpp_lib_optional_range_support != 202406L"
#endif
#if defined(__cpp_lib_out_ptr) && __cpp_lib_out_ptr != 202311L
#  error "__cpp_lib_out_ptr != 202311L"
#endif
#if defined(__cpp_lib_parallel_algorithm) && __cpp_lib_parallel_algorithm != 202506L
#  error "__cpp_lib_parallel_algorithm != 202506L"
#endif
#if defined(__cpp_lib_parallel_scheduler) && __cpp_lib_parallel_scheduler != 202506L
#  error "__cpp_lib_parallel_scheduler != 202506L"
#endif
#if defined(__cpp_lib_philox_engine) && __cpp_lib_philox_engine != 202406L
#  error "__cpp_lib_philox_engine != 202406L"
#endif
#if defined(__cpp_lib_pointer_tag_pair) && __cpp_lib_pointer_tag_pair != 202606L
#  error "__cpp_lib_pointer_tag_pair != 202606L"
#endif
#if defined(__cpp_lib_polymorphic) && __cpp_lib_polymorphic != 202502L
#  error "__cpp_lib_polymorphic != 202502L"
#endif
#if defined(__cpp_lib_polymorphic_allocator) && __cpp_lib_polymorphic_allocator != 201902L
#  error "__cpp_lib_polymorphic_allocator != 201902L"
#endif
#if defined(__cpp_lib_print) && __cpp_lib_print != 202406L
#  error "__cpp_lib_print != 202406L"
#endif
#if defined(__cpp_lib_quoted_string_io) && __cpp_lib_quoted_string_io != 201304L
#  error "__cpp_lib_quoted_string_io != 201304L"
#endif
#if defined(__cpp_lib_ranges) && __cpp_lib_ranges != 202406L
#  error "__cpp_lib_ranges != 202406L"
#endif
#if defined(__cpp_lib_ranges_as_const) && __cpp_lib_ranges_as_const != 202311L
#  error "__cpp_lib_ranges_as_const != 202311L"
#endif
#if defined(__cpp_lib_ranges_as_input) && __cpp_lib_ranges_as_input != 202502L
#  error "__cpp_lib_ranges_as_input != 202502L"
#endif
#if defined(__cpp_lib_ranges_as_rvalue) && __cpp_lib_ranges_as_rvalue != 202207L
#  error "__cpp_lib_ranges_as_rvalue != 202207L"
#endif
#if defined(__cpp_lib_ranges_cache_latest) && __cpp_lib_ranges_cache_latest != 202411L
#  error "__cpp_lib_ranges_cache_latest != 202411L"
#endif
#if defined(__cpp_lib_ranges_cartesian_product) && __cpp_lib_ranges_cartesian_product != 202207L
#  error "__cpp_lib_ranges_cartesian_product != 202207L"
#endif
#if defined(__cpp_lib_ranges_chunk) && __cpp_lib_ranges_chunk != 202202L
#  error "__cpp_lib_ranges_chunk != 202202L"
#endif
#if defined(__cpp_lib_ranges_chunk_by) && __cpp_lib_ranges_chunk_by != 202202L
#  error "__cpp_lib_ranges_chunk_by != 202202L"
#endif
#if defined(__cpp_lib_ranges_concat) && __cpp_lib_ranges_concat != 202403L
#  error "__cpp_lib_ranges_concat != 202403L"
#endif
#if defined(__cpp_lib_ranges_contains) && __cpp_lib_ranges_contains != 202207L
#  error "__cpp_lib_ranges_contains != 202207L"
#endif
#if defined(__cpp_lib_ranges_enumerate) && __cpp_lib_ranges_enumerate != 202302L
#  error "__cpp_lib_ranges_enumerate != 202302L"
#endif
#if defined(__cpp_lib_ranges_filter) && __cpp_lib_ranges_filter != 202603L
#  error "__cpp_lib_ranges_filter != 202603L"
#endif
#if defined(__cpp_lib_ranges_find_last) && __cpp_lib_ranges_find_last != 202207L
#  error "__cpp_lib_ranges_find_last != 202207L"
#endif
#if defined(__cpp_lib_ranges_fold) && __cpp_lib_ranges_fold != 202207L
#  error "__cpp_lib_ranges_fold != 202207L"
#endif
#if defined(__cpp_lib_ranges_generate_random) && __cpp_lib_ranges_generate_random != 202403L
#  error "__cpp_lib_ranges_generate_random != 202403L"
#endif
#if defined(__cpp_lib_ranges_indices) && __cpp_lib_ranges_indices != 202506L
#  error "__cpp_lib_ranges_indices != 202506L"
#endif
#if defined(__cpp_lib_ranges_iota) && __cpp_lib_ranges_iota != 202202L
#  error "__cpp_lib_ranges_iota != 202202L"
#endif
#if defined(__cpp_lib_ranges_join_with) && __cpp_lib_ranges_join_with != 202202L
#  error "__cpp_lib_ranges_join_with != 202202L"
#endif
#if defined(__cpp_lib_ranges_repeat) && __cpp_lib_ranges_repeat != 202207L
#  error "__cpp_lib_ranges_repeat != 202207L"
#endif
#if defined(__cpp_lib_ranges_reserve_hint) && __cpp_lib_ranges_reserve_hint != 202502L
#  error "__cpp_lib_ranges_reserve_hint != 202502L"
#endif
#if defined(__cpp_lib_ranges_slide) && __cpp_lib_ranges_slide != 202202L
#  error "__cpp_lib_ranges_slide != 202202L"
#endif
#if defined(__cpp_lib_ranges_starts_ends_with) && __cpp_lib_ranges_starts_ends_with != 202106L
#  error "__cpp_lib_ranges_starts_ends_with != 202106L"
#endif
#if defined(__cpp_lib_ranges_stride) && __cpp_lib_ranges_stride != 202207L
#  error "__cpp_lib_ranges_stride != 202207L"
#endif
#if defined(__cpp_lib_ranges_to_container) && __cpp_lib_ranges_to_container != 202202L
#  error "__cpp_lib_ranges_to_container != 202202L"
#endif
#if defined(__cpp_lib_ranges_zip) && __cpp_lib_ranges_zip != 202110L
#  error "__cpp_lib_ranges_zip != 202110L"
#endif
#if defined(__cpp_lib_ratio) && __cpp_lib_ratio != 202306L
#  error "__cpp_lib_ratio != 202306L"
#endif
#if defined(__cpp_lib_raw_memory_algorithms) && __cpp_lib_raw_memory_algorithms != 202411L
#  error "__cpp_lib_raw_memory_algorithms != 202411L"
#endif
#if defined(__cpp_lib_rcu) && __cpp_lib_rcu != 202306L
#  error "__cpp_lib_rcu != 202306L"
#endif
#if defined(__cpp_lib_reference_from_temporary) && __cpp_lib_reference_from_temporary != 202202L
#  error "__cpp_lib_reference_from_temporary != 202202L"
#endif
#if defined(__cpp_lib_reference_wrapper) && __cpp_lib_reference_wrapper != 202403L
#  error "__cpp_lib_reference_wrapper != 202403L"
#endif
#if defined(__cpp_lib_reflection) && __cpp_lib_reflection != 202603L
#  error "__cpp_lib_reflection != 202603L"
#endif
#if defined(__cpp_lib_remove_cvref) && __cpp_lib_remove_cvref != 201711L
#  error "__cpp_lib_remove_cvref != 201711L"
#endif
#if defined(__cpp_lib_replaceable_contract_violation_handler) && !(__cpp_lib_replaceable_contract_violation_handler == 202603L || __cpp_lib_replaceable_contract_violation_handler == 0)
#  error "__cpp_lib_replaceable_contract_violation_handler: neither 202603L nor 0"
#endif
#if defined(__cpp_lib_result_of_sfinae) && __cpp_lib_result_of_sfinae != 201210L
#  error "__cpp_lib_result_of_sfinae != 201210L"
#endif
#if defined(__cpp_lib_robust_nonmodifying_seq_ops) && __cpp_lib_robust_nonmodifying_seq_ops != 201304L
#  error "__cpp_lib_robust_nonmodifying_seq_ops != 201304L"
#endif
#if defined(__cpp_lib_sample) && __cpp_lib_sample != 201603L
#  error "__cpp_lib_sample != 201603L"
#endif
#if defined(__cpp_lib_saturation_arithmetic) && __cpp_lib_saturation_arithmetic != 202603L
#  error "__cpp_lib_saturation_arithmetic != 202603L"
#endif
#if defined(__cpp_lib_scoped_lock) && __cpp_lib_scoped_lock != 201703L
#  error "__cpp_lib_scoped_lock != 201703L"
#endif
#if defined(__cpp_lib_semaphore) && __cpp_lib_semaphore != 201907L
#  error "__cpp_lib_semaphore != 201907L"
#endif
#if defined(__cpp_lib_senders) && __cpp_lib_senders != 202506L
#  error "__cpp_lib_senders != 202506L"
#endif
#if defined(__cpp_lib_shared_mutex) && __cpp_lib_shared_mutex != 201505L
#  error "__cpp_lib_shared_mutex != 201505L"
#endif
#if defined(__cpp_lib_shared_ptr_arrays) && __cpp_lib_shared_ptr_arrays != 201707L
#  error "__cpp_lib_shared_ptr_arrays != 201707L"
#endif
#if defined(__cpp_lib_shared_ptr_weak_type) && __cpp_lib_shared_ptr_weak_type != 201606L
#  error "__cpp_lib_shared_ptr_weak_type != 201606L"
#endif
#if defined(__cpp_lib_shared_timed_mutex) && __cpp_lib_shared_timed_mutex != 201402L
#  error "__cpp_lib_shared_timed_mutex != 201402L"
#endif
#if defined(__cpp_lib_shift) && __cpp_lib_shift != 202202L
#  error "__cpp_lib_shift != 202202L"
#endif
#if defined(__cpp_lib_simd) && __cpp_lib_simd != 202606L
#  error "__cpp_lib_simd != 202606L"
#endif
#if defined(__cpp_lib_simd_bitops) && __cpp_lib_simd_bitops != 202607L
#  error "__cpp_lib_simd_bitops != 202607L"
#endif
#if defined(__cpp_lib_simd_complex) && __cpp_lib_simd_complex != 202502L
#  error "__cpp_lib_simd_complex != 202502L"
#endif
#if defined(__cpp_lib_simd_permutations) && __cpp_lib_simd_permutations != 202506L
#  error "__cpp_lib_simd_permutations != 202506L"
#endif
#if defined(__cpp_lib_smart_ptr_for_overwrite) && __cpp_lib_smart_ptr_for_overwrite != 202002L
#  error "__cpp_lib_smart_ptr_for_overwrite != 202002L"
#endif
#if defined(__cpp_lib_smart_ptr_owner_equality) && __cpp_lib_smart_ptr_owner_equality != 202306L
#  error "__cpp_lib_smart_ptr_owner_equality != 202306L"
#endif
#if defined(__cpp_lib_source_location) && __cpp_lib_source_location != 201907L
#  error "__cpp_lib_source_location != 201907L"
#endif
#if defined(__cpp_lib_span) && __cpp_lib_span != 202311L
#  error "__cpp_lib_span != 202311L"
#endif
#if defined(__cpp_lib_spanstream) && __cpp_lib_spanstream != 202106L
#  error "__cpp_lib_spanstream != 202106L"
#endif
#if defined(__cpp_lib_ssize) && __cpp_lib_ssize != 201902L
#  error "__cpp_lib_ssize != 201902L"
#endif
#if defined(__cpp_lib_sstream_from_string_view) && __cpp_lib_sstream_from_string_view != 202306L
#  error "__cpp_lib_sstream_from_string_view != 202306L"
#endif
#if defined(__cpp_lib_stacktrace) && __cpp_lib_stacktrace != 202011L
#  error "__cpp_lib_stacktrace != 202011L"
#endif
#if defined(__cpp_lib_start_lifetime) && __cpp_lib_start_lifetime != 202603L
#  error "__cpp_lib_start_lifetime != 202603L"
#endif
#if defined(__cpp_lib_start_lifetime_as) && __cpp_lib_start_lifetime_as != 202207L
#  error "__cpp_lib_start_lifetime_as != 202207L"
#endif
#if defined(__cpp_lib_starts_ends_with) && __cpp_lib_starts_ends_with != 201711L
#  error "__cpp_lib_starts_ends_with != 201711L"
#endif
#if defined(__cpp_lib_stdatomic_h) && __cpp_lib_stdatomic_h != 202011L
#  error "__cpp_lib_stdatomic_h != 202011L"
#endif
#if defined(__cpp_lib_stdbit_h) && __cpp_lib_stdbit_h != 202603L
#  error "__cpp_lib_stdbit_h != 202603L"
#endif
#if defined(__cpp_lib_stdckdint_h) && __cpp_lib_stdckdint_h != 202603L
#  error "__cpp_lib_stdckdint_h != 202603L"
#endif
#if defined(__cpp_lib_string_contains) && __cpp_lib_string_contains != 202011L
#  error "__cpp_lib_string_contains != 202011L"
#endif
#if defined(__cpp_lib_string_resize_and_overwrite) && __cpp_lib_string_resize_and_overwrite != 202110L
#  error "__cpp_lib_string_resize_and_overwrite != 202110L"
#endif
#if defined(__cpp_lib_string_subview) && __cpp_lib_string_subview != 202506L
#  error "__cpp_lib_string_subview != 202506L"
#endif
#if defined(__cpp_lib_string_udls) && __cpp_lib_string_udls != 201304L
#  error "__cpp_lib_string_udls != 201304L"
#endif
#if defined(__cpp_lib_string_view) && __cpp_lib_string_view != 202403L
#  error "__cpp_lib_string_view != 202403L"
#endif
#if defined(__cpp_lib_submdspan) && __cpp_lib_submdspan != 202603L
#  error "__cpp_lib_submdspan != 202603L"
#endif
#if defined(__cpp_lib_syncbuf) && __cpp_lib_syncbuf != 201803L
#  error "__cpp_lib_syncbuf != 201803L"
#endif
#if defined(__cpp_lib_task) && __cpp_lib_task != 202506L
#  error "__cpp_lib_task != 202506L"
#endif
#if defined(__cpp_lib_text_encoding) && __cpp_lib_text_encoding != 202306L
#  error "__cpp_lib_text_encoding != 202306L"
#endif
#if defined(__cpp_lib_thread_attributes) && __cpp_lib_thread_attributes != 202606L
#  error "__cpp_lib_thread_attributes != 202606L"
#endif
#if defined(__cpp_lib_three_way_comparison) && __cpp_lib_three_way_comparison != 201907L
#  error "__cpp_lib_three_way_comparison != 201907L"
#endif
#if defined(__cpp_lib_to_address) && __cpp_lib_to_address != 201711L
#  error "__cpp_lib_to_address != 201711L"
#endif
#if defined(__cpp_lib_to_array) && __cpp_lib_to_array != 201907L
#  error "__cpp_lib_to_array != 201907L"
#endif
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars != 202606L
#  error "__cpp_lib_to_chars != 202606L"
#endif
#if defined(__cpp_lib_to_string) && __cpp_lib_to_string != 202306L
#  error "__cpp_lib_to_string != 202306L"
#endif
#if defined(__cpp_lib_to_underlying) && __cpp_lib_to_underlying != 202102L
#  error "__cpp_lib_to_underlying != 202102L"
#endif
#if defined(__cpp_lib_transformation_trait_aliases) && __cpp_lib_transformation_trait_aliases != 201304L
#  error "__cpp_lib_transformation_trait_aliases != 201304L"
#endif
#if defined(__cpp_lib_transparent_operators) && __cpp_lib_transparent_operators != 201510L
#  error "__cpp_lib_transparent_operators != 201510L"
#endif
#if defined(__cpp_lib_tuple_element_t) && __cpp_lib_tuple_element_t != 201402L
#  error "__cpp_lib_tuple_element_t != 201402L"
#endif
#if defined(__cpp_lib_tuple_like) && __cpp_lib_tuple_like != 202311L
#  error "__cpp_lib_tuple_like != 202311L"
#endif
#if defined(__cpp_lib_tuples_by_type) && __cpp_lib_tuples_by_type != 201304L
#  error "__cpp_lib_tuples_by_type != 201304L"
#endif
#if defined(__cpp_lib_type_identity) && __cpp_lib_type_identity != 201806L
#  error "__cpp_lib_type_identity != 201806L"
#endif
#if defined(__cpp_lib_type_order) && __cpp_lib_type_order != 202506L
#  error "__cpp_lib_type_order != 202506L"
#endif
#if defined(__cpp_lib_type_trait_variable_templates) && __cpp_lib_type_trait_variable_templates != 201510L
#  error "__cpp_lib_type_trait_variable_templates != 201510L"
#endif
#if defined(__cpp_lib_uncaught_exceptions) && __cpp_lib_uncaught_exceptions != 201411L
#  error "__cpp_lib_uncaught_exceptions != 201411L"
#endif
#if defined(__cpp_lib_unordered_map_try_emplace) && __cpp_lib_unordered_map_try_emplace != 201411L
#  error "__cpp_lib_unordered_map_try_emplace != 201411L"
#endif
#if defined(__cpp_lib_unreachable) && __cpp_lib_unreachable != 202202L
#  error "__cpp_lib_unreachable != 202202L"
#endif
#if defined(__cpp_lib_unwrap_ref) && __cpp_lib_unwrap_ref != 201811L
#  error "__cpp_lib_unwrap_ref != 201811L"
#endif
#if defined(__cpp_lib_valarray) && __cpp_lib_valarray != 202511L
#  error "__cpp_lib_valarray != 202511L"
#endif
#if defined(__cpp_lib_variant) && __cpp_lib_variant != 202306L
#  error "__cpp_lib_variant != 202306L"
#endif
#if defined(__cpp_lib_view_interface) && __cpp_lib_view_interface != 202606L
#  error "__cpp_lib_view_interface != 202606L"
#endif
#if defined(__cpp_lib_void_t) && __cpp_lib_void_t != 201411L
#  error "__cpp_lib_void_t != 201411L"
#endif
