// [utility.underlying]: template<class T> constexpr underlying_type_t<T> to_underlying(T value)
// noexcept; -- [meta.trans.other]: underlying_type<T> has a member type only for an
// enumeration type ("otherwise, there is no member type"), so substitution fails and
// to_underlying cannot be called with a non-enumeration argument. T is deduced by value, so a
// const enumeration argument yields the unqualified underlying type.
#include <utility>
#include <type_traits>

enum class E : long { a = 3 };
enum U : unsigned char { u = 200 };
struct S {
  operator E() const { return E::a; }
};

template <class T>
concept CanToUnderlying = requires(T t) { std::to_underlying(t); };
static_assert(CanToUnderlying<E>);
static_assert(CanToUnderlying<const E>);
static_assert(CanToUnderlying<U&>);
static_assert(!CanToUnderlying<int>);
static_assert(!CanToUnderlying<bool>);
static_assert(!CanToUnderlying<S>);  // no conversion: T is deduced as S
static_assert(!CanToUnderlying<int*>);

constexpr const E ce = E::a;
static_assert(std::is_same_v<decltype(std::to_underlying(ce)), long>);
static_assert(std::to_underlying(ce) == 3L);
static_assert(std::to_underlying(u) == 200);
template <long N>
struct Tag {};
static_assert(sizeof(Tag<std::to_underlying(E::a)>) == 1);  // a constant expression
