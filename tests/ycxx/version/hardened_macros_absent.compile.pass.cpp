// [version.syn]/7 (recommended practice): a non-hardened implementation does not define the
// macros of [version.syn]/3. __cpp_lib_hardened_shared_ptr_array, _basic_stacktrace and
// _view_interface are defined only with YCXX_HARDENED=1 (hardened_macros.compile.pass.cpp).
// REQUIRES: !hardened
#include <memory>
#include <ranges>
#include <stacktrace>

#ifdef __cpp_lib_hardened_shared_ptr_array
#  error "__cpp_lib_hardened_shared_ptr_array is defined without YCXX_HARDENED"
#endif
#ifdef __cpp_lib_hardened_basic_stacktrace
#  error "__cpp_lib_hardened_basic_stacktrace is defined without YCXX_HARDENED"
#endif
#ifdef __cpp_lib_hardened_view_interface
#  error "__cpp_lib_hardened_view_interface is defined without YCXX_HARDENED"
#endif
