// [meta.unary.prop] Table 54: is_swappable_with<T, U>: "The expressions swap(declval<T>(),
// declval<U>()) and swap(declval<U>(), declval<T>()) are each well-formed when treated as an
// unevaluated operand in an overload-resolution context for swappable values
// ([swappable.requirements]). Access checking is performed as if in a context unrelated to T
// and U. Only the validity of the immediate context of the swap expressions is considered."
// is_swappable<T>: "For a referenceable type T, the same result as is_swappable_with_v<T&, T&>,
// otherwise false." is_nothrow_swappable_with: is_swappable_with_v and "each expression is known
// not to throw"; is_nothrow_swappable likewise.
// [swappable.requirements]/3: the candidate set is the two std::swap templates plus ADL.
// [utility.swap]: std::swap(T&, T&) is constrained on is_move_constructible_v<T> &&
// is_move_assignable_v<T>, noexcept(is_nothrow_move_constructible_v<T> &&
// is_nothrow_move_assignable_v<T>); swap(T(&)[N], T(&)[N]) constrained on is_swappable_v<T>,
// noexcept(is_nothrow_swappable_v<T>).
// COUNTERPART: libcxx:utilities/meta/meta.unary/meta.unary.prop/is_swappable.pass.cpp
// COUNTERPART: libstdcxx:20_util/(optional/swap/2|pair/swap_cxx17|tuple/swap_cxx17|unique_ptr/specialized_algorithms/swap_cxx17).cc
// COUNTERPART: libstdcxx:23_containers/array/specialized_algorithms/swap_cxx17.cc
#include <type_traits>
#include <utility>

namespace adl {
struct A {};
struct B {};
void swap(A&, B&);           // heterogeneous, may throw
void swap(B&, A&) noexcept;
struct OnlyOneWay {};
void swap(OnlyOneWay&, int&);
struct NoMove {             // not movable, but has an ADL swap
  NoMove(NoMove&&) = delete;
  NoMove& operator=(NoMove&&) = delete;
};
void swap(NoMove&, NoMove&) noexcept;
struct NoexceptSwap {
  NoexceptSwap(NoexceptSwap&&);              // may throw
  NoexceptSwap& operator=(NoexceptSwap&&);
};
void swap(NoexceptSwap&, NoexceptSwap&) noexcept;
struct DeletedSwap {};
void swap(DeletedSwap&, DeletedSwap&) = delete;   // ambiguous with/preferred over std::swap: ill-formed
struct RvalueSwap {};
void swap(RvalueSwap&&, RvalueSwap&&) noexcept;   // swappable as rvalues
class PrivateSwap {
  friend void swap(PrivateSwap&, PrivateSwap&) noexcept;   // hidden friend: found by ADL, accessible
};
}  // namespace adl

struct Plain {};
struct ThrowingMove {
  ThrowingMove(ThrowingMove&&);
  ThrowingMove& operator=(ThrowingMove&&) noexcept;
};
struct ThrowingAssign {
  ThrowingAssign(ThrowingAssign&&) noexcept;
  ThrowingAssign& operator=(ThrowingAssign&&);
};
struct CopyOnly {
  CopyOnly(const CopyOnly&);
  CopyOnly& operator=(const CopyOnly&);
};
struct Abstract { virtual void f() = 0; };
struct Incomplete;
enum E { e };
enum class SE { s };

template <class T, class U> constexpr bool sw = std::is_swappable_with_v<T, U> && std::is_swappable_with<T, U>::value &&
    std::is_base_of_v<std::true_type, std::is_swappable_with<T, U>>;
template <class T, class U> constexpr bool nsw = !std::is_swappable_with_v<T, U> && !std::is_nothrow_swappable_with_v<T, U> &&
    std::is_base_of_v<std::false_type, std::is_swappable_with<T, U>>;

// fundamental and compound types
static_assert(std::is_swappable_v<int> && std::is_nothrow_swappable_v<int>);
static_assert(std::is_swappable_v<int*> && std::is_swappable_v<std::nullptr_t> && std::is_swappable_v<E> && std::is_swappable_v<SE>);
static_assert(std::is_swappable_v<int Plain::*> && std::is_swappable_v<void (*)()>);
static_assert(!std::is_swappable_v<const int> && !std::is_swappable_v<const int[2]>);
static_assert(std::is_swappable_v<int&> && std::is_swappable_v<int&&>);   // is_swappable_with<int&, int&>
static_assert(!std::is_swappable_v<const int&>);
static_assert(!std::is_swappable_v<void> && !std::is_swappable_v<const void>);
static_assert(!std::is_swappable_v<void()> && !std::is_swappable_v<void() const> && !std::is_nothrow_swappable_v<void() &>);
static_assert(!std::is_swappable_v<void (&)()>);   // functions are not assignable
static_assert(std::is_swappable_v<int[3]> && std::is_swappable_v<int[2][3]> && std::is_nothrow_swappable_v<int[2][3]>);
static_assert(!std::is_swappable_v<int[]>);   // no swap for arrays of unknown bound
static_assert(!std::is_swappable_v<Abstract>);
static_assert(sw<int&, int&> && nsw<int, int> && nsw<int&, int> && nsw<int&, long&> && nsw<int&, const int&>);
static_assert(sw<int (&)[2], int (&)[2]> && nsw<int (&)[2], int (&)[3]> && nsw<int (&)[2], long (&)[2]>);
static_assert(nsw<int&, void> && nsw<void, void>);

// class types through std::swap
static_assert(std::is_swappable_v<Plain> && std::is_nothrow_swappable_v<Plain>);
static_assert(std::is_swappable_v<CopyOnly> && !std::is_nothrow_swappable_v<CopyOnly>);
static_assert(std::is_swappable_v<ThrowingMove> && !std::is_nothrow_swappable_v<ThrowingMove>);
static_assert(std::is_swappable_v<ThrowingAssign> && !std::is_nothrow_swappable_v<ThrowingAssign>);
static_assert(std::is_swappable_v<ThrowingMove[2]> && !std::is_nothrow_swappable_v<ThrowingMove[2]>);
static_assert(!std::is_nothrow_swappable_v<ThrowingAssign[1][2]>);
static_assert(sw<Plain&, Plain&> && nsw<Plain, Plain> && nsw<Plain&, const Plain&>);

// ADL customization
static_assert(sw<adl::A&, adl::B&> && sw<adl::B&, adl::A&>);
static_assert(!std::is_nothrow_swappable_with_v<adl::A&, adl::B&>);   // swap(A&, B&) may throw
static_assert(!std::is_nothrow_swappable_with_v<adl::B&, adl::A&>);   // both directions are required
static_assert(nsw<adl::OnlyOneWay&, int&>);                         // swap(int&, OnlyOneWay&) missing
static_assert(std::is_swappable_v<adl::NoMove> && std::is_nothrow_swappable_v<adl::NoMove>);
static_assert(std::is_swappable_v<adl::NoMove[3]> && std::is_nothrow_swappable_v<adl::NoMove[3]>);
static_assert(std::is_nothrow_swappable_v<adl::NoexceptSwap>);
static_assert(!std::is_swappable_v<adl::DeletedSwap>);
static_assert(!std::is_swappable_v<adl::DeletedSwap[2]>);
static_assert(sw<adl::RvalueSwap, adl::RvalueSwap> && std::is_nothrow_swappable_with_v<adl::RvalueSwap, adl::RvalueSwap>);
static_assert(std::is_swappable_v<adl::RvalueSwap>);   // lvalues: std::swap applies (movable)
static_assert(std::is_swappable_v<adl::PrivateSwap> && std::is_nothrow_swappable_v<adl::PrivateSwap>);

// _v agrees with ::value and the base characteristic, for every trait.
template <class T> constexpr bool agree1 =
    std::is_swappable_v<T> == std::is_swappable<T>::value &&
    std::is_nothrow_swappable_v<T> == std::is_nothrow_swappable<T>::value &&
    std::is_base_of_v<std::bool_constant<std::is_swappable_v<T>>, std::is_swappable<T>> &&
    std::is_base_of_v<std::bool_constant<std::is_nothrow_swappable_v<T>>, std::is_nothrow_swappable<T>>;
static_assert(agree1<int> && agree1<const int> && agree1<void> && agree1<int[]> && agree1<adl::A> && agree1<ThrowingMove>);
static_assert(agree1<void() const> && agree1<int&> && agree1<adl::DeletedSwap>);
template <class T, class U> constexpr bool agree2 =
    std::is_swappable_with_v<T, U> == std::is_swappable_with<T, U>::value &&
    std::is_nothrow_swappable_with_v<T, U> == std::is_nothrow_swappable_with<T, U>::value &&
    std::is_base_of_v<std::bool_constant<std::is_nothrow_swappable_with_v<T, U>>, std::is_nothrow_swappable_with<T, U>>;
static_assert(agree2<adl::A&, adl::B&> && agree2<int, int> && agree2<int&, int&> && agree2<void, void>);

int main() {}
