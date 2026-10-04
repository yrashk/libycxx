// [meta.trans.other]/4: common_type.
//   (4.1) sizeof...(T) == 0: no member type.
//   (4.2) one type T0: "the same type, if any, as common_type_t<T0, T0>" (so a program-defined
//         specialization common_type<X, X> is used for common_type<X> and common_type<const X>).
//   (4.3) two types: D1, D2 = decay_t<T1>, decay_t<T2>;
//         (4.3.1) if T1 != D1 or T2 != D2: common_type_t<D1, D2> (a user specialization is found);
//         (4.3.3) else decay_t<decltype(false ? declval<D1>() : declval<D2>())>;
//         (4.3.4) else decay_t<COND-RES(CREF(D1), CREF(D2))>.
//   (4.4) more than two: common_type_t<common_type_t<T1, T2>, R...>, no member if any step fails.
// /5: a program may specialize common_type<T1, T2> for decayed program-defined types.
// [meta.trans.other]/3.4: COND-RES(X, Y) = decltype(false ? declval<X(&)()>()() : declval<Y(&)()>()()).
#include <type_traits>
#include <cstddef>

template <class A, class B> constexpr bool same = std::is_same_v<A, B>;
template <class... T> concept HasCT = requires { typename std::common_type<T...>::type; };

struct Base {};
struct Derived : Base {};
struct Other {};
struct ToInt { operator int() const; };
struct Expl { explicit operator int() const; };
struct Incomplete;

// User specializations ([meta.trans.other]/5).
struct UA {};
struct UB {};
struct UResult { UResult(UA); UResult(UB); };
struct USelf {};
struct USelfResult { USelfResult(USelf); };
struct UNoType {};
template <> struct std::common_type<UA, UB> { using type = UResult; };
template <> struct std::common_type<UB, UA> { using type = UResult; };
template <> struct std::common_type<USelf, USelf> { using type = USelfResult; };
template <> struct std::common_type<UNoType, int> {};
template <> struct std::common_type<int, UNoType> {};
// UNoType converts to int; without the empty specializations the conditional operator would work.

// (4.1)
static_assert(!HasCT<>);
// (4.2)
static_assert(same<std::common_type_t<int>, int>);
static_assert(same<std::common_type_t<const int>, int>);
static_assert(same<std::common_type_t<volatile int&>, int>);
static_assert(same<std::common_type_t<int[3]>, int*>);
static_assert(same<std::common_type_t<const int[]>, const int*>);
static_assert(same<std::common_type_t<void(int)>, void (*)(int)>);
static_assert(same<std::common_type_t<void>, void>);
static_assert(same<std::common_type_t<const volatile void>, void>);
static_assert(same<std::common_type_t<Base>, Base>);
static_assert(same<std::common_type_t<const Base&>, Base>);
static_assert(same<std::common_type_t<USelf>, USelfResult>);           // via common_type<USelf, USelf>
static_assert(same<std::common_type_t<const USelf&>, USelfResult>);    // via (4.3.1)
static_assert(same<std::common_type_t<USelf, USelf>, USelfResult>);
static_assert(same<std::common_type_t<volatile USelf, const USelf&&>, USelfResult>);
// (4.3.1) decay first, then the user specialization.
static_assert(same<std::common_type_t<UA, UB>, UResult>);
static_assert(same<std::common_type_t<const UA, UB>, UResult>);
static_assert(same<std::common_type_t<UA&, const UB&&>, UResult>);
static_assert(same<std::common_type_t<UB, volatile UA>, UResult>);
static_assert(!HasCT<UNoType, int>);
static_assert(!HasCT<const UNoType&, int>);
static_assert(!HasCT<UNoType, const int&>);
// (4.3.3)
static_assert(same<std::common_type_t<int, int>, int>);
static_assert(same<std::common_type_t<int, long>, long>);
static_assert(same<std::common_type_t<char, short>, int>);
static_assert(same<std::common_type_t<unsigned, int>, unsigned>);
static_assert(same<std::common_type_t<float, int>, float>);
static_assert(same<std::common_type_t<float, double>, double>);
static_assert(same<std::common_type_t<const int&, volatile long&&>, long>);
static_assert(same<std::common_type_t<bool, bool>, bool>);
static_assert(same<std::common_type_t<void, void>, void>);
static_assert(same<std::common_type_t<const void, volatile void>, void>);
static_assert(!HasCT<void, int>);
static_assert(!HasCT<int, void>);
static_assert(same<std::common_type_t<int*, const int*>, const int*>);
static_assert(same<std::common_type_t<int*, std::nullptr_t>, int*>);
static_assert(same<std::common_type_t<std::nullptr_t, std::nullptr_t>, std::nullptr_t>);
static_assert(same<std::common_type_t<int*, void*>, void*>);
static_assert(same<std::common_type_t<Derived*, Base*>, Base*>);
static_assert(same<std::common_type_t<const Derived*, Base*>, const Base*>);
static_assert(same<std::common_type_t<int[3], int*>, int*>);
static_assert(same<std::common_type_t<int[3], const int[4]>, const int*>);
static_assert(same<std::common_type_t<void(), void (*)()>, void (*)()>);
static_assert(same<std::common_type_t<void() noexcept, void()>, void (*)()>);
static_assert(same<std::common_type_t<int Derived::*, int Base::*>, int Derived::*>);
static_assert(same<std::common_type_t<int Base::*, std::nullptr_t>, int Base::*>);
static_assert(same<std::common_type_t<Base, Derived>, Base>);
static_assert(same<std::common_type_t<Derived, const Base&>, Base>);
static_assert(same<std::common_type_t<ToInt, int>, int>);
static_assert(same<std::common_type_t<int, ToInt>, int>);
static_assert(!HasCT<Expl, int>);
static_assert(!HasCT<Base, Other>);
static_assert(!HasCT<int, Base>);
static_assert(!HasCT<int*, long*>);
static_assert(!HasCT<int, int*>);
enum E { e };
enum class SE { s };
static_assert(same<std::common_type_t<E, E>, E>);
static_assert(same<std::common_type_t<SE, const SE&>, SE>);
static_assert(same<std::common_type_t<E, int>, int>);
static_assert(!HasCT<SE, int>);
// The conditional operator of two xvalues of the same class type: decay gives the class type
// even when it is not movable (no object is created in an unevaluated operand).
struct NoMove { NoMove(NoMove&&) = delete; };
static_assert(same<std::common_type_t<NoMove, NoMove>, NoMove>);
static_assert(same<std::common_type_t<NoMove>, NoMove>);
// (4.3.4): the prvalue conditional fails, the const-lvalue one works.
struct Src {};
struct Dst { Dst(const Src&); Dst(Src&&) = delete; };
static_assert(same<std::common_type_t<Dst, Src>, Dst>);
static_assert(same<std::common_type_t<Src, Dst>, Dst>);
// A class type converting only from an lvalue of the other.
struct FromLvalue { FromLvalue(Base&); };
static_assert(!HasCT<FromLvalue, Base>);   // CREF gives const Base&, which does not bind to Base&
// (4.4)
static_assert(same<std::common_type_t<char, short, int>, int>);
static_assert(same<std::common_type_t<int, long, float, double>, double>);
static_assert(same<std::common_type_t<Derived, Base, const Base&>, Base>);
static_assert(same<std::common_type_t<UA, UB, UResult>, UResult>);
static_assert(same<std::common_type_t<USelf, USelf, USelf>, USelfResult>);   // then <USelfResult, USelf>
static_assert(!HasCT<int, int, void>);
static_assert(!HasCT<void, void, int>);
static_assert(!HasCT<int, Base, Base>);
static_assert(!HasCT<UNoType, int, int>);
static_assert(same<std::common_type_t<void, void, const void>, void>);
static_assert(same<std::common_type_t<int, int, int, int, int, int, int, int, long>, long>);
// SFINAE-friendliness inside a template.
template <class... T> constexpr bool probe() {
  if constexpr (requires { typename std::common_type_t<T...>; }) return true;
  else return false;
}
static_assert(probe<int, long>() && !probe<int, Base>() && !probe<>());
// The trait accepts an array of unknown bound and an incomplete type in (4.1)-(4.3) only via
// decay: common_type<Incomplete[]> decays to Incomplete*.
static_assert(same<std::common_type_t<Incomplete[]>, Incomplete*>);
static_assert(same<std::common_type_t<Incomplete[], Incomplete*>, Incomplete*>);

int main() {}
