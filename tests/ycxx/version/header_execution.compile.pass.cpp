// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <execution> in their comment. Only <execution> is included.
#include <execution>

#if !defined(__cpp_lib_execution)
#  error "__cpp_lib_execution is not defined"
#elif __cpp_lib_execution != 201902L
#  error "__cpp_lib_execution != 201902L"
#endif
#if !defined(__cpp_lib_senders)
#  error "__cpp_lib_senders is not defined"
#elif __cpp_lib_senders != 202506L
#  error "__cpp_lib_senders != 202506L"
#endif
#if !defined(__cpp_lib_counting_scope)
#  error "__cpp_lib_counting_scope is not defined"
#elif __cpp_lib_counting_scope != 202506L
#  error "__cpp_lib_counting_scope != 202506L"
#endif
#if !defined(__cpp_lib_parallel_scheduler)
#  error "__cpp_lib_parallel_scheduler is not defined"
#elif __cpp_lib_parallel_scheduler != 202506L
#  error "__cpp_lib_parallel_scheduler != 202506L"
#endif
#if !defined(__cpp_lib_task)
#  error "__cpp_lib_task is not defined"
#elif __cpp_lib_task != 202506L
#  error "__cpp_lib_task != 202506L"
#endif
