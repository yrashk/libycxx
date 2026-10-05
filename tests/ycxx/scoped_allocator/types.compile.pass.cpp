// [allocator.adaptor.syn], [allocator.adaptor.types]: scoped_allocator_adaptor<Outer, Inner...>
// derives from Outer; its member types come from allocator_traits<Outer>; inner_allocator_type
// is scoped_allocator_adaptor<Outer> with no inner allocators, else
// scoped_allocator_adaptor<Inner...>; POCCA/POCMA/POCS are true if true for any allocator;
// is_always_equal is true if true for all; rebind replaces only the outer allocator; the
// deduction guide deduces from the constructor arguments.
// REQUIRES: exceptions
#include <scoped_allocator>
#include <memory>
#include <type_traits>
#include "test_allocators.hpp"

using std::scoped_allocator_adaptor;
using std::is_same_v;

using S1 = scoped_allocator_adaptor<std::allocator<int>>;
static_assert(std::is_base_of_v<std::allocator<int>, S1>);
static_assert(is_same_v<S1::outer_allocator_type, std::allocator<int>>);
static_assert(is_same_v<S1::inner_allocator_type, S1>);
static_assert(is_same_v<S1::value_type, int> && is_same_v<S1::pointer, int*> && is_same_v<S1::const_pointer, const int*>);
static_assert(is_same_v<S1::size_type, std::size_t> && is_same_v<S1::difference_type, std::ptrdiff_t>);
static_assert(is_same_v<S1::void_pointer, void*> && is_same_v<S1::const_void_pointer, const void*>);
static_assert(S1::is_always_equal::value && !S1::propagate_on_container_copy_assignment::value);
static_assert(S1::propagate_on_container_move_assignment::value);  // std::allocator's POCMA

using S3 = scoped_allocator_adaptor<IdAlloc<int>, IdAlloc<char, true>, IdAlloc<long, false, false, true>>;
static_assert(is_same_v<S3::inner_allocator_type,
                        scoped_allocator_adaptor<IdAlloc<char, true>, IdAlloc<long, false, false, true>>>);
static_assert(is_same_v<S3::inner_allocator_type::inner_allocator_type, scoped_allocator_adaptor<IdAlloc<long, false, false, true>>>);
static_assert(S3::propagate_on_container_copy_assignment::value);
static_assert(!S3::propagate_on_container_move_assignment::value);
static_assert(S3::propagate_on_container_swap::value);
static_assert(!S3::is_always_equal::value);
static_assert(std::is_same_v<S3::propagate_on_container_swap, std::true_type>);
static_assert(std::is_same_v<S3::is_always_equal, std::false_type>);

static_assert(is_same_v<S3::rebind<double>::other,
                        scoped_allocator_adaptor<IdAlloc<double>, IdAlloc<char, true>, IdAlloc<long, false, false, true>>>);
static_assert(is_same_v<std::allocator_traits<S1>::rebind_alloc<char>, scoped_allocator_adaptor<std::allocator<char>>>);

static_assert(is_same_v<decltype(scoped_allocator_adaptor(std::allocator<int>(), IdAlloc<char>())),
                        scoped_allocator_adaptor<std::allocator<int>, IdAlloc<char>>>);
static_assert(std::is_nothrow_copy_constructible_v<S3> && std::is_nothrow_move_constructible_v<S3>);
static_assert(std::is_constructible_v<S3, IdAlloc<int>, IdAlloc<char, true>, IdAlloc<long, false, false, true>>);
static_assert(!std::is_constructible_v<S3, int*, IdAlloc<char, true>, IdAlloc<long, false, false, true>>);
// Converting from an adaptor with a different outer type.
static_assert(std::is_constructible_v<scoped_allocator_adaptor<IdAlloc<int>, IdAlloc<char>>,
                                      const scoped_allocator_adaptor<IdAlloc<double>, IdAlloc<char>>&>);
