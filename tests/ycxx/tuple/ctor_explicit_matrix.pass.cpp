// [tuple.cnstr]: explicit(...) of every tuple constructor, per value category of the source:
//   /8   tuple(): explicit iff some Ti is not copy-list-initializable from an empty list.
//   /11  tuple(const Types&...): explicit iff !conjunction_v<is_convertible<const Types&, Types>...>.
//   /15  tuple(UTypes&&...): explicit iff !conjunction_v<is_convertible<UTypes, Types>...>.
//   /21, /23 tuple(tuple<UTypes...>& / const& / && / const&&): constrained on
//        is_constructible_v<Types, decltype(get<I>(FWD(u)))>, and (21.3) for one element, on
//        is_convertible_v<decltype(u), T>, is_constructible_v<T, decltype(u)>, is_same_v<T, U> all
//        false; explicit iff !(is_convertible_v<decltype(get<I>(FWD(u))), Types> && ...).
//   /25, /27 tuple(pair<U1, U2> in all four categories): likewise.
//   /29, /31 tuple(UTuple&&) for tuple-like UTuple (here array<int, N>): likewise, (29.5) for one
//        element: is_convertible_v<UTuple, T> and is_constructible_v<T, UTuple> both false.
//   /33 the allocator-extended forms have the same explicit conditions.
// [pairs.pair]/14-17: pair(P&&) from a pair-like P (tuple<U1, U2>, array<U, 2>), explicit iff
//   either get<i>(FWD(p)) is not convertible.
// A type whose implicit conversion depends on the value category (OnlyLv: implicit from int&,
// explicit from const int&) tells the four overloads apart.
// COUNTERPART: libcxx:utilities/tuple/tuple.tuple/tuple.cnstr/default.pass.cpp
#include <tuple>
#include <utility>
#include <array>
#include <memory>
#include <type_traits>
#include "check.hpp"

struct OnlyLv {            // copy-initializable only from a non-const lvalue int
  int v;
  constexpr OnlyLv(int& i) : v(i) {}
  constexpr explicit OnlyLv(const int& i) : v(i + 100) {}
};
struct Ex { int v; constexpr explicit Ex(int i) : v(i) {} };
struct Im { int v; constexpr Im(int i) : v(i) {} };
struct ExDef { explicit ExDef() = default; };
struct ExCopy { ExCopy() = default; explicit ExCopy(const ExCopy&) = default; };

template <class T> void take(T);
template <class T, class... A> concept Implicit = requires(A&&... a) { take<T>({static_cast<A&&>(a)...}); };
template <class T, class... A> concept Explicit = std::is_constructible_v<T, A...> && !Implicit<T, A...>;
template <class T, class... A> concept None = !std::is_constructible_v<T, A...>;

using std::tuple;
using std::pair;
using std::array;

// /8
static_assert(Implicit<tuple<>> && Implicit<tuple<int, Im*>>);
static_assert(Explicit<tuple<ExDef>> && Explicit<tuple<int, ExDef>> && None<tuple<int, Ex>>);
// /11
static_assert(Implicit<tuple<int, Im>, const int&, const Im&>);
static_assert(Explicit<tuple<ExCopy>, const ExCopy&> && Explicit<tuple<int, ExCopy>, int, const ExCopy&>);
// /15
static_assert(Implicit<tuple<Im, Im>, int, int> && Explicit<tuple<Im, Ex>, int, int> && Explicit<tuple<Ex>, int>);
static_assert(Implicit<tuple<OnlyLv>, int&> && Explicit<tuple<OnlyLv>, const int&> && Explicit<tuple<OnlyLv>, int>);
static_assert(Explicit<tuple<OnlyLv, Im>, int&&, int>);
static_assert(None<tuple<int, int>, int> && None<tuple<int>, int, int>);
// /21-/23: tuple<OnlyLv> from tuple<int> in the four categories.
static_assert(Implicit<tuple<OnlyLv>, tuple<int>&>);
static_assert(Explicit<tuple<OnlyLv>, const tuple<int>&>);
static_assert(Explicit<tuple<OnlyLv>, tuple<int>&&>);
static_assert(Explicit<tuple<OnlyLv>, const tuple<int>&&>);
static_assert(Implicit<tuple<OnlyLv, Im>, tuple<int, int>&>);
static_assert(Explicit<tuple<OnlyLv, Im>, tuple<int, int>>);
static_assert(Explicit<tuple<Im, Ex>, tuple<int, int>&>);
static_assert(Implicit<tuple<Im, Im>, const tuple<int, int>&&>);
static_assert(Implicit<tuple<long, double>, tuple<int, float>&> && Implicit<tuple<long, double>, const tuple<int, float>&&>);
static_assert(None<tuple<int, int>, tuple<int>&> && None<tuple<int, int*>, tuple<int, int>&>);
// /25-/27: from pair.
static_assert(Implicit<tuple<OnlyLv, OnlyLv>, pair<int, int>&>);
static_assert(Explicit<tuple<OnlyLv, OnlyLv>, const pair<int, int>&>);
static_assert(Explicit<tuple<OnlyLv, Im>, pair<int, int>&&>);
static_assert(Explicit<tuple<Im, OnlyLv>, const pair<int, int>&&>);
static_assert(Implicit<tuple<Im, long>, const pair<int, int>&&>);
static_assert(None<tuple<int, int, int>, pair<int, int>> && None<tuple<int>, pair<int, int>>);
// /29-/31: from a tuple-like (array).
static_assert(Implicit<tuple<OnlyLv, OnlyLv>, array<int, 2>&>);
static_assert(Explicit<tuple<OnlyLv, OnlyLv>, const array<int, 2>&>);
static_assert(Explicit<tuple<OnlyLv, OnlyLv>, array<int, 2>>);
static_assert(Implicit<tuple<Im, long, double>, array<int, 3>>);
static_assert(Explicit<tuple<Im, Ex, Im>, array<int, 3>&>);
static_assert(Implicit<tuple<OnlyLv>, array<int, 1>&> && Explicit<tuple<OnlyLv>, array<int, 1>>);
static_assert(None<tuple<int, int>, array<int, 3>> && None<tuple<int, int>, array<int*, 2>>);
// /33: allocator-extended.
using A = std::allocator<int>;
static_assert(Implicit<tuple<Im, Im>, std::allocator_arg_t, const A&, int, int>);
static_assert(Explicit<tuple<Im, Ex>, std::allocator_arg_t, const A&, int, int>);
static_assert(Explicit<tuple<ExDef>, std::allocator_arg_t, const A&>);
static_assert(Implicit<tuple<OnlyLv>, std::allocator_arg_t, const A&, tuple<int>&>);
static_assert(Explicit<tuple<OnlyLv>, std::allocator_arg_t, const A&, tuple<int>&&>);
static_assert(Implicit<tuple<OnlyLv, Im>, std::allocator_arg_t, const A&, pair<int, int>&>);
static_assert(Explicit<tuple<OnlyLv, Im>, std::allocator_arg_t, const A&, const pair<int, int>&>);
static_assert(Implicit<tuple<OnlyLv, OnlyLv>, std::allocator_arg_t, const A&, array<int, 2>&>);
static_assert(Explicit<tuple<OnlyLv, OnlyLv>, std::allocator_arg_t, const A&, array<int, 2>&&>);
// [pairs.pair]/14-17: pair from pair-like.
static_assert(Implicit<pair<OnlyLv, OnlyLv>, tuple<int, int>&>);
static_assert(Explicit<pair<OnlyLv, OnlyLv>, const tuple<int, int>&>);
static_assert(Explicit<pair<OnlyLv, Im>, tuple<int, int>>);
static_assert(Implicit<pair<OnlyLv, OnlyLv>, array<int, 2>&>);
static_assert(Explicit<pair<OnlyLv, OnlyLv>, const array<int, 2>&&>);
static_assert(Implicit<pair<Im, long>, array<int, 2>>);
static_assert(Explicit<pair<OnlyLv, OnlyLv>, const pair<int, int>&>);
static_assert(Implicit<pair<OnlyLv, OnlyLv>, pair<int, int>&>);
static_assert(None<pair<int, int>, tuple<int, int, int>> && None<pair<int, int>, array<int, 3>>);

// (21.3)/(29.5): a single element constructible from the whole source tuple uses UTypes&&.
struct FromAnything {
  int which = 0;
  constexpr FromAnything() = default;
  constexpr FromAnything(int) : which(1) {}
  template <class T> requires (!std::is_same_v<std::remove_cvref_t<T>, FromAnything> &&
                               !std::is_same_v<std::remove_cvref_t<T>, int>)
  constexpr FromAnything(T&&) : which(2) {}
};

constexpr bool run() {
  int i = 7;
  const int ci = 8;
  // The element is direct-initialized from get<I>(FWD(u)): explicit ctor of OnlyLv for const.
  tuple<int> src(i);
  tuple<OnlyLv> a(src);
  if (std::get<0>(a).v != 7) return false;
  tuple<OnlyLv> b(static_cast<const tuple<int>&>(src));
  if (std::get<0>(b).v != 107) return false;
  tuple<OnlyLv> c(std::move(src));
  if (std::get<0>(c).v != 107) return false;
  tuple<OnlyLv> d(ci);
  if (std::get<0>(d).v != 108) return false;
  tuple<OnlyLv> e = {i};
  if (std::get<0>(e).v != 7) return false;
  pair<int, int> p(1, 2);
  tuple<OnlyLv, OnlyLv> f = p;
  if (std::get<0>(f).v != 1 || std::get<1>(f).v != 2) return false;
  tuple<OnlyLv, OnlyLv> g(std::as_const(p));
  if (std::get<0>(g).v != 101 || std::get<1>(g).v != 102) return false;
  array<int, 2> arr{3, 4};
  tuple<OnlyLv, OnlyLv> h = arr;
  if (std::get<0>(h).v != 3 || std::get<1>(h).v != 4) return false;
  pair<OnlyLv, OnlyLv> q = arr;
  if (q.first.v != 3 || q.second.v != 4) return false;
  pair<OnlyLv, OnlyLv> r(std::move(arr));
  if (r.first.v != 103 || r.second.v != 104) return false;
  // (21.3): tuple<FromAnything>(tuple<int>) initializes the element from the whole tuple.
  tuple<int> ti(5);
  tuple<FromAnything> fa(ti);
  if (std::get<0>(fa).which != 2) return false;
  tuple<FromAnything> fb(std::move(ti));
  if (std::get<0>(fb).which != 2) return false;
  // (29.5): likewise for a tuple-like source.
  array<int, 1> a1{1};
  tuple<FromAnything> fc(a1);
  if (std::get<0>(fc).which != 2) return false;
  return true;
}
static_assert(run());

int main() { CHECK(run()); }
