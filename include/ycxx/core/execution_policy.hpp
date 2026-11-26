// libycxx core: execution policies ([execpol]). libycxx runs every parallel algorithm overload
// sequentially on the calling thread, which all four standard policies permit.
#pragma once

#include <ycxx/core/type_traits.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

#include <ycxx/core/execution_policy.inc>

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// The constraint of the parallel algorithm overloads ([algorithms.parallel.overloads]).
template <class _Ep>
concept __execution_policy = std::is_execution_policy_v<std::remove_cvref_t<_Ep>>;
}} // namespace __ycxx::__detail
