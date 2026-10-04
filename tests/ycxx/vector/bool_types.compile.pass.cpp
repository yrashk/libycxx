// [vector.bool.pspc]/1: vector<bool, Allocator> has value_type bool, const_reference bool,
// a nested class reference (copy constructible, assignable from bool and from reference,
// const-assignable from bool, convertible to bool, flip(), and three friend swaps), and
// iterators whose reference type is that reference ([container.reqmts]: forward iterators;
// as a sequence with operator[] they are random access). /3: storage need not be an array
// of bool. All reference members are noexcept.
#include <vector>
#include <iterator>
#include <type_traits>
#include <utility>

using VB = std::vector<bool>;
using R = VB::reference;
static_assert(std::is_same_v<VB::value_type, bool>);
static_assert(std::is_same_v<VB::const_reference, bool>);
static_assert(std::is_same_v<VB::allocator_type, std::allocator<bool>>);
static_assert(std::is_class_v<R>);
static_assert(std::is_same_v<decltype(std::declval<VB&>()[0]), R>);
static_assert(std::is_same_v<decltype(std::declval<const VB&>()[0]), bool>);
static_assert(std::is_same_v<decltype(std::declval<VB&>().at(0)), R>);
static_assert(std::is_same_v<decltype(std::declval<const VB&>().front()), bool>);
static_assert(std::is_same_v<decltype(std::declval<VB&>().back()), R>);
static_assert(std::is_same_v<std::iter_reference_t<VB::iterator>, R>);
static_assert(std::is_same_v<std::iter_value_t<VB::iterator>, bool>);
static_assert(std::random_access_iterator<VB::iterator>);
static_assert(std::random_access_iterator<VB::const_iterator>);
static_assert(std::is_convertible_v<VB::iterator, VB::const_iterator>);
static_assert(std::is_same_v<VB::reverse_iterator, std::reverse_iterator<VB::iterator>>);
static_assert(std::is_unsigned_v<VB::size_type> && std::is_signed_v<VB::difference_type>);

static_assert(std::is_nothrow_copy_constructible_v<R>);
static_assert(std::is_nothrow_assignable_v<R&, bool>);
static_assert(std::is_nothrow_assignable_v<R&, const R&>);
static_assert(std::is_nothrow_assignable_v<const R&, bool>);
static_assert(std::is_nothrow_convertible_v<R, bool>);
static_assert(std::is_same_v<decltype(std::declval<R&>() = true), R&>);
static_assert(std::is_same_v<decltype(std::declval<const R&>() = true), const R&>);
static_assert(noexcept(std::declval<R&>().flip()));
static_assert(std::is_same_v<decltype(std::declval<R&>().flip()), void>);
static_assert(noexcept(swap(std::declval<R>(), std::declval<R>())));
static_assert(noexcept(swap(std::declval<R>(), std::declval<bool&>())));
static_assert(noexcept(swap(std::declval<bool&>(), std::declval<R>())));
static_assert(std::indirectly_writable<VB::iterator, bool>);  // uses the const operator=
static_assert(std::output_iterator<VB::iterator, bool>);

static_assert(noexcept(std::declval<VB&>().flip()));
static_assert(std::is_same_v<decltype(std::declval<VB&>().flip()), void>);
static_assert(std::is_nothrow_move_constructible_v<VB>);
static_assert(std::is_nothrow_default_constructible_v<VB>);
static_assert(std::is_nothrow_swappable_v<VB>);
