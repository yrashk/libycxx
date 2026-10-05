// libycxx core: pieces shared by the ordered associative containers and the flat container
// adaptors ([associative.reqmts], [container.adaptors]): the transparent-comparator tests and the
// exposition-only alias templates of their deduction guides ([associative.general]/2).
#pragma once

#include <ycxx/core/container_base.hpp>
#include <ycxx/core/pair.hpp>
#include <ycxx/core/sequence_support.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template <class Compare>
concept transparent_compare = requires { typename Compare::is_transparent; };

// [associative.reqmts.general]/180: the heterogeneous erase and extract (and other members with
// the same constraint) do not take arguments convertible to the container's iterators.
template <class Compare, class K, class It, class CIt>
concept transparent_non_iter =
    transparent_compare<Compare> && !std::is_convertible_v<K&&, It> && !std::is_convertible_v<K&&, CIt>;

template <class I>
using iter_key_type = std::remove_cvref_t<std::tuple_element_t<0, iter_value_type<I>>>;
template <class I>
using iter_mapped_type = std::remove_cvref_t<std::tuple_element_t<1, iter_value_type<I>>>;
template <class I>
using iter_to_alloc_type = std::pair<const iter_key_type<I>, iter_mapped_type<I>>;
template <class R>
using range_key_type = std::remove_cvref_t<std::tuple_element_t<0, std::ranges::range_value_t<R>>>;
template <class R>
using range_mapped_type = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<R>>>;
template <class R>
using range_to_alloc_type = std::pair<const range_key_type<R>, range_mapped_type<R>>;

// A deduced Compare must not qualify as an allocator ([associative.reqmts.general]/181,
// [container.adaptors.general]/6).
template <class C>
concept deducible_compare = !qualifies_as_allocator<C>;

}} // namespace ycxx::detail
