// [mem.res], [allocator.adaptor]: memory_resource, polymorphic_allocator (new_object,
// allocate_bytes, ...), the standard resources, pool_options, scoped_allocator_adaptor.
#include <memory_resource>
#include <scoped_allocator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace pmr = std::pmr;
static_assert(std::is_abstract_v<pmr::memory_resource> && std::has_virtual_destructor_v<pmr::memory_resource>);
static_assert(std::is_same_v<decltype(std::declval<pmr::memory_resource&>().allocate(1)), void*>);
static_assert(noexcept(std::declval<const pmr::memory_resource&>().is_equal(std::declval<const pmr::memory_resource&>())));
static_assert(noexcept(std::declval<pmr::memory_resource&>() == std::declval<pmr::memory_resource&>()));
static_assert(noexcept(pmr::new_delete_resource()) && noexcept(pmr::null_memory_resource()) &&
              noexcept(pmr::get_default_resource()) && noexcept(pmr::set_default_resource(nullptr)));
using PA = pmr::polymorphic_allocator<>;
static_assert(std::is_same_v<PA, pmr::polymorphic_allocator<std::byte>>);
static_assert(std::is_nothrow_default_constructible_v<PA> && std::is_convertible_v<pmr::memory_resource*, PA>);
static_assert(!std::is_copy_assignable_v<PA>);
static_assert(std::is_same_v<decltype(std::declval<PA&>().allocate_bytes(1)), void*>);
static_assert(std::is_same_v<decltype(std::declval<PA&>().allocate_object<int>(2)), int*>);
static_assert(std::is_same_v<decltype(std::declval<PA&>().new_object<int>(1)), int*>);
static_assert(std::is_same_v<decltype(std::declval<PA&>().select_on_container_copy_construction()), PA>);
static_assert(std::is_same_v<decltype(std::declval<const PA&>().resource()), pmr::memory_resource*>);
static_assert(std::is_same_v<pmr::vector<int>, std::vector<int, pmr::polymorphic_allocator<int>>>);
static_assert(std::is_aggregate_v<pmr::pool_options> || std::is_default_constructible_v<pmr::pool_options>);
static_assert(std::is_base_of_v<pmr::memory_resource, pmr::synchronized_pool_resource> &&
              std::is_base_of_v<pmr::memory_resource, pmr::unsynchronized_pool_resource> &&
              std::is_base_of_v<pmr::memory_resource, pmr::monotonic_buffer_resource>);
static_assert(!std::is_copy_constructible_v<pmr::monotonic_buffer_resource>);
static_assert(std::is_constructible_v<pmr::monotonic_buffer_resource, void*, std::size_t>);
static_assert(std::is_same_v<decltype(std::declval<const pmr::unsynchronized_pool_resource&>().options()), pmr::pool_options>);
// [allocator.adaptor]
using SA = std::scoped_allocator_adaptor<std::allocator<int>, std::allocator<char>>;
static_assert(std::is_same_v<SA::outer_allocator_type, std::allocator<int>> &&
              std::is_same_v<SA::inner_allocator_type, std::scoped_allocator_adaptor<std::allocator<char>>>);
static_assert(std::is_same_v<SA::rebind<long>::other, std::scoped_allocator_adaptor<std::allocator<long>, std::allocator<char>>>);
static_assert(noexcept(std::declval<SA&>().inner_allocator()) && noexcept(std::declval<SA&>().outer_allocator()));
static_assert(std::is_same_v<decltype(std::scoped_allocator_adaptor(std::allocator<int>())), std::scoped_allocator_adaptor<std::allocator<int>>>);
static_assert(std::is_same_v<decltype(SA() == SA()), bool>);
