// [pairs.pair]/13 and /17: the dangling-reference cases are "defined as deleted", not removed
// from the overload set by a constraint. [dcl.fct.def.delete]/2 and [over.match.general]/3: a
// deleted function takes part in overload resolution; using it is ill-formed only once
// selected. So when a pair<const int&, int> conversion would dangle, a call that could convert
// the argument either to that pair or to another parameter type is ambiguous, rather than
// silently choosing the other overload.
#include <utility>
#include <tuple>

using CRef = std::pair<const int&, int>;

struct FromLongPair {
  FromLongPair(const std::pair<long, int>&);
  FromLongPair(std::pair<long, int>&&);
};
void g(CRef);
void g(FromLongPair);
template <class T>
concept g_callable = requires(T&& t) { g(static_cast<T&&>(t)); };

// the pair conversion is implicit here (is_convertible_v<const long&, const int&> is true)
static_assert(!g_callable<const std::pair<long, int>&>);
static_assert(!g_callable<std::pair<long, int>&&>);
static_assert(!g_callable<std::pair<long, int>&>);

struct FromLongTuple {
  FromLongTuple(const std::tuple<long, int>&);
};
void h(CRef);
void h(FromLongTuple);
template <class T>
concept h_callable = requires(T&& t) { h(static_cast<T&&>(t)); };
static_assert(!h_callable<const std::tuple<long, int>&>);

struct FromTwoLongs {
  FromTwoLongs(long, int);
};
void k(CRef);
void k(FromTwoLongs);
template <class A, class B>
concept k_callable = requires(A a, B b) { k({a, b}); };
static_assert(!k_callable<long, int>);  // both list-initializations are viable

// control: with a non-dangling source only the pair overload is viable / better
struct Unrelated {};
void m(CRef);
void m(Unrelated);
template <class T>
concept m_callable = requires(T&& t) { m(static_cast<T&&>(t)); };
static_assert(m_callable<std::pair<int, int>&>);
