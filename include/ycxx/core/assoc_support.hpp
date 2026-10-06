// libycxx core: pieces shared by the ordered associative containers and the flat container
// adaptors ([associative.reqmts], [container.adaptors]): the transparent-comparator tests and the
// exposition-only alias templates of their deduction guides ([associative.general]/2).
#pragma once

#include <ycxx/core/container_base.hpp>
#include <ycxx/core/pair.hpp>
#include <ycxx/core/sequence_support.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Compare>
concept __transparent_compare = requires { typename _Compare::is_transparent; };

// [associative.reqmts.general]/180: the heterogeneous erase and extract (and other members with
// the same constraint) do not take arguments convertible to the container's iterators.
template <class _Compare, class _Kp, class _It, class _CIt>
concept __transparent_non_iter =
    __transparent_compare<_Compare> && !std::is_convertible_v<_Kp&&, _It> && !std::is_convertible_v<_Kp&&, _CIt>;

template <class _Ip>
using __iter_key_type = std::remove_cvref_t<std::tuple_element_t<0, __iter_value_type<_Ip>>>;
template <class _Ip>
using __iter_mapped_type = std::remove_cvref_t<std::tuple_element_t<1, __iter_value_type<_Ip>>>;
template <class _Ip>
using __iter_to_alloc_type = std::pair<const __iter_key_type<_Ip>, __iter_mapped_type<_Ip>>;
template <class _Rp>
using __range_key_type = std::remove_cvref_t<std::tuple_element_t<0, std::ranges::range_value_t<_Rp>>>;
template <class _Rp>
using __range_mapped_type = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<_Rp>>>;
template <class _Rp>
using __range_to_alloc_type = std::pair<const __range_key_type<_Rp>, __range_mapped_type<_Rp>>;

// A deduced Compare must not qualify as an allocator ([associative.reqmts.general]/181,
// [container.adaptors.general]/6).
template <class _Cp>
concept __deducible_compare = !__qualifies_as_allocator<_Cp>;

}} // namespace __ycxx::__detail
