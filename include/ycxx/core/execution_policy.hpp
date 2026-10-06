// libycxx core: execution policies ([execpol]). libycxx runs every parallel algorithm overload
// sequentially on the calling thread, which all four standard policies permit.
#pragma once

#include <ycxx/core/type_traits.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp>
struct is_execution_policy : false_type {};
template <class _Tp>
constexpr bool is_execution_policy_v = is_execution_policy<_Tp>::value;

namespace execution {
class sequenced_policy {};
class parallel_policy {};
class parallel_unsequenced_policy {};
class unsequenced_policy {};

inline constexpr sequenced_policy seq{};
inline constexpr parallel_policy par{};
inline constexpr parallel_unsequenced_policy par_unseq{};
inline constexpr unsequenced_policy unseq{};
} // namespace execution

template <>
struct is_execution_policy<execution::sequenced_policy> : true_type {};
template <>
struct is_execution_policy<execution::parallel_policy> : true_type {};
template <>
struct is_execution_policy<execution::parallel_unsequenced_policy> : true_type {};
template <>
struct is_execution_policy<execution::unsequenced_policy> : true_type {};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// The constraint of the parallel algorithm overloads ([algorithms.parallel.overloads]).
template <class _Ep>
concept __execution_policy = std::is_execution_policy_v<std::remove_cvref_t<_Ep>>;
}} // namespace __ycxx::__detail
