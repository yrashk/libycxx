// [associative.reqmts.general]/175: "The insert, insert_range, and emplace members shall not
// affect the validity of iterators and references to the container". This includes the
// past-the-end iterator ([container.reqmts]/31) of a map / multimap with static storage
// duration that has never held an element: such an object's default initialization
// ([map.cons], [multimap.cons]: the default constructor is constexpr) may be constant
// initialization ([basic.start.static]), so its first insertion in main may be the first
// change of a constant-initialized object. end(), cend(), begin(), rbegin() and failed
// lookups are saved before the first insertion; afterwards they are still the past-the-end
// iterator: equal to end(), std::prev of them is the element, they work as hints
// ([associative.reqmts.general]/72, /58), as the start of reverse iteration and as the end of
// erase(begin(), e) (/134-137). Objects at namespace scope (external and internal linkage)
// and a function-local static are covered; for map also a first insertion by try_emplace,
// insert_or_assign ([map.modifiers]) and operator[] ([map.access]).
#include <map>
#include <iterator>
#include <memory>
#include "container_values.hpp"
#include "check.hpp"

template <class X>
typename X::key_type key(int i) {
  return val<typename X::key_type>(i);
}
template <class X>
typename X::value_type elem(int i) {
  return typename X::value_type(val<typename X::key_type>(i), val<typename X::mapped_type>(i));
}

// g has static storage duration and has never held an element. Fills it for the first time
// and checks the iterators saved before that.
template <class X>
void first_fill(X& g, const char* name) {
  using It = typename X::iterator;
  CHECK_SAY(g.empty() && g.begin() == g.end(), "%s", name);
  const It e = g.end();
  const typename X::const_iterator ce = g.cend();
  const It b = g.begin();
  const It f = g.find(key<X>(9));
  const It lb = g.lower_bound(key<X>(9));
  const typename X::reverse_iterator rb = g.rbegin();
  g.insert(elem<X>(1));
  CHECK_SAY(e == g.end(), "%s", name);
  CHECK_SAY(ce == g.cend(), "%s", name);
  CHECK_SAY(b == g.end(), "%s", name);
  CHECK_SAY(f == g.end(), "%s", name);
  CHECK_SAY(lb == g.end(), "%s", name);
  CHECK_SAY(std::prev(e) == g.begin(), "%s", name);
  CHECK_SAY(*std::prev(e) == elem<X>(1), "%s", name);
  CHECK_SAY(rb == g.rbegin(), "%s", name);
  CHECK_SAY(std::addressof(*rb) == std::addressof(*g.begin()), "%s", name);
  CHECK_SAY(g.find(key<X>(9)) == e, "%s", name);
  // the saved end as a hint
  const It h = g.insert(e, elem<X>(2));
  CHECK_SAY(*h == elem<X>(2), "%s", name);
  CHECK_SAY(std::next(h) == e && std::prev(e) == h, "%s", name);
  const It h2 = g.emplace_hint(ce, elem<X>(3));
  CHECK_SAY(*h2 == elem<X>(3), "%s", name);
  CHECK_SAY(std::next(h2) == e && std::prev(h2) == h, "%s", name);
  // reverse iteration from the saved end
  int n = 0;
  for (typename X::reverse_iterator r(e); r != g.rend(); ++r) ++n;
  CHECK_SAY(n == 3, "%s", name);
  // the saved end as the end of an erased range
  CHECK_SAY(g.erase(g.begin(), e) == e, "%s", name);
  CHECK_SAY(g.empty() && g.end() == e && g.begin() == e, "%s", name);
  g.insert(elem<X>(4));
  CHECK_SAY(std::prev(e) == g.begin() && g.end() == e, "%s", name);
  g.clear();
  CHECK_SAY(g.empty(), "%s", name);
}

// The first insertion is made by f(g).
template <class X, class F>
void first_fill_by(X& g, F f, const char* name) {
  CHECK_SAY(g.empty(), "%s", name);
  const auto e = g.end();
  const auto ce = g.cend();
  f(g);
  CHECK_SAY(e == g.end() && ce == g.cend(), "%s", name);
  CHECK_SAY(std::prev(e) == g.begin(), "%s", name);
  CHECK_SAY(*std::prev(e) == elem<X>(1), "%s", name);
  const auto h = g.try_emplace(e, key<X>(2), val<typename X::mapped_type>(2));
  CHECK_SAY(std::next(h) == e && std::prev(e) == h, "%s", name);
  CHECK_SAY(g.erase(g.begin(), e) == e && g.empty() && g.end() == e, "%s", name);
}

std::map<int, int> global_map;
std::multimap<int, int> global_multimap;
static std::map<Elem, Elem> internal_map;
namespace {
std::multimap<Elem, int> unnamed_multimap;
}
std::map<int, int> by_try_emplace, by_insert_or_assign, by_subscript;

std::map<long, long>& local_map() {
  static std::map<long, long> m;
  return m;
}

int main() {
  first_fill(global_map, "global map<int, int>");
  first_fill(global_multimap, "global multimap<int, int>");
  first_fill(internal_map, "static map<Elem, Elem>");
  first_fill(unnamed_multimap, "unnamed-namespace multimap<Elem, int>");
  first_fill(local_map(), "function-local static map<long, long>");
  using M = std::map<int, int>;
  first_fill_by(by_try_emplace, [](M& m) { m.try_emplace(key<M>(1), val<int>(1)); }, "try_emplace");
  first_fill_by(by_insert_or_assign, [](M& m) { m.insert_or_assign(key<M>(1), val<int>(1)); }, "insert_or_assign");
  first_fill_by(by_subscript, [](M& m) { m[key<M>(1)] = val<int>(1); }, "operator[]");
  return 0;
}
