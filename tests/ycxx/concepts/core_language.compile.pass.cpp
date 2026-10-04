// [concept.same]/1 "same_as<T, U> subsumes same_as<U, T> and vice versa."
// [concept.derived]: derived_from<Derived, Base> = is_base_of_v<Base, Derived> &&
// is_convertible_v<const volatile Derived*, const volatile Base*> ("publicly and
// unambiguously derived ... or the same class type ignoring cv-qualifiers").
// [concept.convertible]: convertible_to requires both implicit and explicit conversion.
// [concepts.arithmetic]: unsigned_integral = integral<T> && !signed_integral<T> (so bool).
#include <concepts>
#include <cstddef>
#include <type_traits>

// same_as subsumption: the second overload must be more constrained than the first.
template <class T, class U>
  requires std::same_as<T, U>
constexpr int f() { return 1; }
template <class T, class U>
  requires std::same_as<U, T> && std::integral<T>
constexpr int f() { return 2; }
static_assert(f<int, int>() == 2);
static_assert(f<double, double>() == 1);

struct Base {};
struct Pub : Base {};
struct Priv : private Base {};
struct Prot : protected Base {};
struct L : Pub {};
struct R : Pub {};
struct Diamond : L, R {};  // ambiguous Pub/Base
struct Virt1 : virtual Base {};
struct Virt2 : virtual Base {};
struct VDiamond : Virt1, Virt2 {};
union U {};

static_assert(std::derived_from<Pub, Base>);
static_assert(std::derived_from<L, Base>);
static_assert(std::derived_from<Base, Base>);
static_assert(std::derived_from<const Pub, volatile Base>);
static_assert(!std::derived_from<Priv, Base>);
static_assert(!std::derived_from<Prot, Base>);
static_assert(!std::derived_from<Diamond, Base>);
static_assert(std::derived_from<VDiamond, Base>);
static_assert(!std::derived_from<Base, Pub>);
static_assert(!std::derived_from<int, int>);
static_assert(!std::derived_from<U, U>);
static_assert(!std::derived_from<Pub&, Base&>);

struct ExplicitOnly {
  explicit ExplicitOnly(int) {}
};
struct ImplicitOnlyTo {
  operator int() const;
};
struct NoStaticCast {
  operator int() const;
  explicit operator long() const = delete;
};
static_assert(std::convertible_to<int, long>);
static_assert(std::convertible_to<Pub*, Base*>);
static_assert(!std::convertible_to<Priv*, Base*>);
static_assert(!std::convertible_to<int, ExplicitOnly>);
static_assert(std::convertible_to<ImplicitOnlyTo, int>);
// implicitly convertible to long (via operator int), but static_cast<long> picks the deleted
// explicit operator long, so the explicit conversion is ill-formed
static_assert(std::is_convertible_v<NoStaticCast, long>);
static_assert(!std::convertible_to<NoStaticCast, long>);
static_assert(std::convertible_to<NoStaticCast, int>);
static_assert(std::convertible_to<int, void> == false);
static_assert(std::convertible_to<void, void>);
static_assert(std::convertible_to<int&, const int&>);
static_assert(!std::convertible_to<const int&, int&>);
static_assert(!std::convertible_to<int[2], int[2]>);
static_assert(std::convertible_to<int (&)[2], int*>);

static_assert(std::integral<bool> && std::integral<char> && std::integral<wchar_t> && std::integral<char8_t>);
static_assert(std::integral<const int> && std::integral<volatile long>);
static_assert(!std::integral<int&> && !std::integral<float> && !std::integral<std::byte>);
static_assert(std::unsigned_integral<bool>);
static_assert(!std::signed_integral<bool>);
static_assert(std::signed_integral<signed char> && std::unsigned_integral<unsigned char>);
static_assert(std::signed_integral<char> == (static_cast<char>(-1) < 0));
static_assert(std::unsigned_integral<char> == !(static_cast<char>(-1) < 0));
static_assert(std::unsigned_integral<char8_t> && std::unsigned_integral<char16_t> && std::unsigned_integral<char32_t>);
static_assert(!std::signed_integral<float> && !std::unsigned_integral<float>);
static_assert(std::floating_point<float> && std::floating_point<const long double>);
static_assert(!std::floating_point<int> && !std::floating_point<double&>);
enum E : int {};
static_assert(!std::integral<E>);
