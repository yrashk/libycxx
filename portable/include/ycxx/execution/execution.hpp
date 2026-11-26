// The libycxx sender/receiver implementation for use with a hosted C++ standard library.
#pragma once

#include <ycxx/execution/detail/standard_headers.hpp>
#include <ycxx/execution/stop_token.hpp>

namespace ycxx::__portable_std {

using namespace ::std;
using ::ycxx::execution::inplace_stop_callback;
using ::ycxx::execution::inplace_stop_source;
using ::ycxx::execution::inplace_stop_token;

#include <ycxx/core/stop_token_traits.inc>

namespace execution {
using std::execution::par;
using std::execution::parallel_policy;
using std::execution::parallel_unsequenced_policy;
}

} // namespace ycxx::__portable_std

namespace __ycxx_portable_exec::__detail {

namespace __cfg {
#ifdef __cpp_exceptions
inline constexpr bool exceptions = true;
#else
inline constexpr bool exceptions = false;
#endif

#ifdef __cpp_lib_constexpr_exceptions
inline constexpr bool __constexpr_exceptions = true;
#else
inline constexpr bool __constexpr_exceptions = false;
#endif

inline constexpr auto __default_new_alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
} // namespace __cfg

template <class _Tp, class _Up>
using __forward_like_t = decltype(std::forward_like<_Tp>(std::declval<_Up&>()));

constexpr void __precondition(bool __condition, const char*) noexcept {
  if (!__condition)
    std::terminate();
}

} // namespace __ycxx_portable_exec::__detail

#include <ycxx/execution/detail/exec_coro.hpp>
#include <ycxx/execution/detail/exec_task.hpp>

namespace ycxx::execution {
using namespace ::ycxx::__portable_std::execution;
using ::ycxx::__portable_std::forwarding_query;
using ::ycxx::__portable_std::forwarding_query_t;
using ::ycxx::__portable_std::get_allocator;
using ::ycxx::__portable_std::get_allocator_t;
using ::ycxx::__portable_std::get_stop_token;
using ::ycxx::__portable_std::get_stop_token_t;
using ::ycxx::__portable_std::never_stop_token;
using ::ycxx::__portable_std::stop_callback_for_t;
using ::ycxx::__portable_std::stop_token_of_t;
using ::ycxx::__portable_std::stoppable_token;
using ::ycxx::__portable_std::unstoppable_token;
using ::ycxx::__portable_std::this_thread::sync_wait;
using ::ycxx::__portable_std::this_thread::sync_wait_with_variant;
} // namespace ycxx::execution
