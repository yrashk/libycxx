// Associative, unordered and flat containers filled from a SOURCE range that throws part-way
// (iterator increment or dereference, filter predicate), with every throw point.
//   [associative.reqmts.except]/2, [unord.req.except]/2: only single-element insertion has the
//   no-effect guarantee; for insert_range, insert(i, j), the from_range / iterator-pair
//   constructors and ranges::to the container must stay valid ([res.on.exception.handling])
//   and every element object constructed must be destroyed exactly once (a live count returns
//   to zero; the container's size() equals its iteration length and the elements alive are
//   exactly those it holds).
//   [flat.map.overview]/6-7, [flat.set.overview]/6-7: "If any member function in [flat.map.defn]
//   exits via an exception, the invariants are restored" (sorted, unique keys; keys and values
//   of equal size).
// REQUIRES: exceptions
#include <cstddef>
#include <flat_map>
#include <flat_set>
#include <functional>
#include <iterator>
#include <map>
#include <ranges>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "check.hpp"

static int live = 0, budget = -1, failures = 0;
static void tick() {
  if (budget >= 0 && budget-- == 0) throw 9;
}
struct E {
  int v;
  E(int x) : v(x) { ++live; }
  E(const E& o) : v(o.v) { ++live; }
  E& operator=(const E&) = default;
  ~E() { --live; }
  friend bool operator==(const E&, const E&) = default;
  friend auto operator<=>(const E&, const E&) = default;
};

template <>
struct std::hash<E> {
  std::size_t operator()(const E& e) const noexcept { return static_cast<std::size_t>(e.v); }
};

static const int keys[8] = {5, 3, 8, 1, 3, 7, 2, 5};  // duplicates

// Single-pass input iterator over keys; ++ and * tick.
template <class V>
struct InK {
  using value_type = V;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::input_iterator_tag;
  using reference = V;
  using pointer = void;
  const int* p = nullptr;
  V operator*() const {
    tick();
    if constexpr (requires { V(E(0), E(0)); }) return V(E(*p), E(*p * 10));
    else return V(*p);
  }
  InK& operator++() {
    tick();
    ++p;
    return *this;
  }
  InK operator++(int) {
    InK t = *this;
    ++*this;
    return t;
  }
  friend bool operator==(const InK& a, const InK& b) { return a.p == b.p; }
};

template <class V>
auto in_src() { return std::ranges::subrange(InK<V>{keys}, InK<V>{keys + 8}); }
template <class V>
auto fwd_src() {
  return std::views::all(keys) | std::views::filter([](int x) {
           tick();
           return x != 7;
         }) |
         std::views::transform([](int x) {
           if constexpr (requires { V(E(0), E(0)); }) return V(E(x), E(x * 10));
           else return V(x);
         });
}

template <class C>
void check_valid(const C& c) {
  std::size_t n = 0;
  std::vector<int> ks;
  for (const auto& e : c) {
    ++n;
    if constexpr (requires { e.first; }) ks.push_back(e.first.v);
    else ks.push_back(e.v);
  }
  CHECK(n == c.size());
  if constexpr (requires { c.key_comp(); }) CHECK(std::ranges::is_sorted(ks));
  constexpr bool unique = requires(C& x, typename C::value_type v) { x.insert(v).second; };
  if constexpr (unique) {
    auto s = ks;
    std::ranges::sort(s);
    CHECK(std::ranges::adjacent_find(s) == s.end());
  }
  for (int k : ks) CHECK(c.contains(E(k)));
}

template <class C, class Op>
void sweep(Op op, int line) {
  bool completed = false;
  for (int limit = 0; limit < 400 && !completed; ++limit) {
    {
      C c;
      constexpr bool is_map = requires { typename C::mapped_type; };
      if constexpr (is_map) c.insert(typename C::value_type(E(100), E(1000)));
      else c.insert(E(100));
      const int base = live;
      const int per = is_map ? 2 : 1;
      budget = limit;
      bool threw = false;
      try {
        op(c);
      } catch (int e) {
        CHECK(e == 9);
        threw = true;
      }
      budget = -1;
      completed = !threw;
      check_valid(c);
      if (live != base + per * (static_cast<int>(c.size()) - 1)) {
        dprintf(2, "line %d: limit %d: %d element objects alive for %zu elements\n", line, limit, live - base + per,
                c.size());
        ++failures;
        live = base + per * (static_cast<int>(c.size()) - 1);
        break;
      }
    }
    if (live != 0) {
      dprintf(2, "line %d: limit %d: %d objects never destroyed\n", line, limit, live);
      ++failures;
      live = 0;
      break;
    }
  }
}

template <class C>
void run(int line) {
  using Src = std::conditional_t<requires { typename C::mapped_type; }, std::pair<E, E>, E>;
  sweep<C>([](C& c) { c.insert_range(in_src<Src>()); }, line * 1000 + __LINE__);
  sweep<C>([](C& c) { c.insert_range(fwd_src<Src>()); }, line * 1000 + __LINE__);
  sweep<C>([](C& c) { auto r = in_src<Src>(); c.insert(r.begin(), r.end()); }, line * 1000 + __LINE__);
  sweep<C>([](C& c) { C d(std::from_range, in_src<Src>()); c.swap(d); }, line * 1000 + __LINE__);
  sweep<C>([](C& c) { C d(std::from_range, fwd_src<Src>()); c.swap(d); }, line * 1000 + __LINE__);
  sweep<C>([](C& c) { auto r = in_src<Src>(); C d(r.begin(), r.end()); c.swap(d); }, line * 1000 + __LINE__);
  sweep<C>([](C& c) { auto d = fwd_src<Src>() | std::ranges::to<C>(); c.swap(d); }, line * 1000 + __LINE__);
}

int main() {
  run<std::map<E, E>>(__LINE__);
  run<std::multimap<E, E>>(__LINE__);
  run<std::set<E>>(__LINE__);
  run<std::multiset<E>>(__LINE__);
  run<std::unordered_map<E, E>>(__LINE__);
  run<std::unordered_multimap<E, E>>(__LINE__);
  run<std::unordered_set<E>>(__LINE__);
  run<std::unordered_multiset<E>>(__LINE__);
  run<std::flat_map<E, E>>(__LINE__);
  run<std::flat_multimap<E, E>>(__LINE__);
  run<std::flat_set<E>>(__LINE__);
  run<std::flat_multiset<E>>(__LINE__);
  CHECK(failures == 0);
  CHECK(live == 0);
  return 0;
}
