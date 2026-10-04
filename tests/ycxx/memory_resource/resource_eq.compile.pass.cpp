// [mem.res.syn]: bool operator==(const memory_resource&, const memory_resource&) noexcept;
// (!= synthesized); template<class T1, class T2> bool operator==(const polymorphic_allocator<T1>&,
// const polymorphic_allocator<T2>&) noexcept. pool_options has the members
// max_blocks_per_chunk and largest_required_pool_block of type size_t.
#include <memory_resource>
#include <cstddef>
#include <type_traits>
#include <utility>

using std::declval;
using MR = std::pmr::memory_resource;
static_assert(std::is_same_v<decltype(declval<const MR&>() == declval<const MR&>()), bool>);
static_assert(noexcept(declval<const MR&>() != declval<const MR&>()));
static_assert(noexcept(declval<std::pmr::polymorphic_allocator<int>>() == declval<std::pmr::polymorphic_allocator<long>>()));
static_assert(std::is_same_v<decltype(std::pmr::pool_options::max_blocks_per_chunk), std::size_t>);
static_assert(std::is_same_v<decltype(std::pmr::pool_options::largest_required_pool_block), std::size_t>);
static_assert(std::is_same_v<decltype(declval<std::pmr::monotonic_buffer_resource&>().upstream_resource()), MR*>);
static_assert(std::is_same_v<decltype(declval<const std::pmr::unsynchronized_pool_resource&>().options()), std::pmr::pool_options>);
