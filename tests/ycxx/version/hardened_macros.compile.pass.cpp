// [version.syn]/3: a hardened implementation defines __cpp_lib_hardened_shared_ptr_array,
// __cpp_lib_hardened_basic_stacktrace and __cpp_lib_hardened_view_interface (202506L), also in
// <memory>, <stacktrace> and <ranges> ([version.syn]/2). libycxx is hardened with
// YCXX_HARDENED=1 (its [util.smartptr.shared.obs], [stacktrace.basic.obs] and
// [view.interface.members] checks).
// FLAGS: -DYCXX_HARDENED=1
#include <memory>
#include <ranges>
#include <stacktrace>

#if !defined(__cpp_lib_hardened_shared_ptr_array)
#  error "__cpp_lib_hardened_shared_ptr_array is not defined"
#elif __cpp_lib_hardened_shared_ptr_array != 202506L
#  error "__cpp_lib_hardened_shared_ptr_array != 202506L"
#endif
#if !defined(__cpp_lib_hardened_basic_stacktrace)
#  error "__cpp_lib_hardened_basic_stacktrace is not defined"
#elif __cpp_lib_hardened_basic_stacktrace != 202506L
#  error "__cpp_lib_hardened_basic_stacktrace != 202506L"
#endif
#if !defined(__cpp_lib_hardened_view_interface)
#  error "__cpp_lib_hardened_view_interface is not defined"
#elif __cpp_lib_hardened_view_interface != 202506L
#  error "__cpp_lib_hardened_view_interface != 202506L"
#endif
