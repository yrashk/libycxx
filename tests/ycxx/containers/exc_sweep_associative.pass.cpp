// Exception-injection sweep over map, set, multimap and multiset: the comparison object, the
// element type's constructors/assignments, the allocator and source iterators throw at their
// k-th call, for every k until the operation completes. After every run all element objects
// are destroyed exactly once and every allocated node deallocated exactly once with its size
// ([res.on.exception.handling]/1, [allocator.requirements.general]).
//   [associative.reqmts.except]/2: "if an exception is thrown by any operation from within an
//     insert or emplace function inserting a single element, the insertion has no effect"
//     (insert(v), insert(hint, v), emplace, emplace_hint, insert(nh), insert(hint, nh), and for
//     map: operator[], try_emplace, insert_or_assign of a new key, which insert one element).
//     For insert(nh) "no effect" includes the node handle: it still owns its element.
//   /1: "erase(k) does not throw an exception unless that exception is thrown by the
//     container's Compare object": basic guarantee for a throwing comparison.
//   Range insertion, constructors, assignments: basic guarantee.
// After every exception the container must still be a valid search tree: its traversal is
// sorted (strictly for unique keys) and find() locates every element.
// REQUIRES: exceptions
#include <map>
#include <set>
#include <utility>
#include "exc_harness.hpp"

using namespace exh;

using Set = std::set<T, exh::less, alloc<T>>;
using MSet = std::multiset<T, exh::less, alloc<T>>;
using Map = std::map<T, T, exh::less, alloc<std::pair<const T, T>>>;
using MMap = std::multimap<T, T, exh::less, alloc<std::pair<const T, T>>>;

template <class C>
constexpr bool is_map = requires { typename C::mapped_type; };
template <class C>
constexpr bool is_multi = !requires(C& c, typename C::value_type v) { c.insert(v).second; };

template <class C>
typename C::value_type val(int k) {
  if constexpr (is_map<C>)
    return {T(k), T(k + 1)};
  else
    return T(k);
}
template <class C>
const T& key_of(const typename C::value_type& v) {
  if constexpr (is_map<C>)
    return v.first;
  else
    return v;
}

template <class C>
C make() {
  C c;
  for (int k : {10, 20, 30, 40, 50}) c.insert(val<C>(k));
  if constexpr (is_multi<C>) c.insert(val<C>(30));
  return c;
}

template <class C>
static typename C::value_type* src() {
  static typename C::value_type s[6] = {val<C>(5), val<C>(30), val<C>(35), val<C>(55), val<C>(15), val<C>(35)};
  return s;
}

template <class C>
constexpr long objs_per_elem = is_map<C> ? 2 : 1;

template <class C>
void check_valid(const C& c) {
  snap s = snap::of(c);
  snap keys;
  keys.n = 0;
  for (auto& v : c) keys.v[keys.n++] = key_of<C>(v).v;
  EXH_EXPECT(keys.is_sorted(!is_multi<C>), "traversal not sorted after an exception");
  EXH_EXPECT(long(c.size()) == s.n, "size() disagrees with traversal");
  long found = 0;
  {
    // find with the comparator disarmed (attempt() has returned)
    for (auto& v : c)
      if (c.find(key_of<C>(v)) != c.end()) ++found;
  }
  EXH_EXPECT(found == s.n, "find() does not locate every element");
}

enum class G { strong, basic };

template <class C, class Op>
void go(const char* cname, const char* opname, G g, std::initializer_list<Kind> kinds, Op op) {
  static char label[160];
  __builtin_snprintf(label, sizeof label, "%s: %s", cname, opname);
  for (Kind k : kinds)
    sweep(label, k, [&] {
      C c = make<C>();
      snap before = snap::of(c);
      long live0 = st.live;
      long n0 = long(c.size());
      bool threw = attempt([&] { op(c); });
      EXH_EXPECT(st.live - live0 == (long(c.size()) - n0) * objs_per_elem<C>,
                 "live element objects do not match the container's size");
      if (threw) {
        check_valid(c);
        if (g == G::strong) EXH_EXPECT(snap::of(c) == before, "single-element insertion had an effect");
      }
      return threw;
    });
}

template <class C>
void run(const char* cname) {
  (void)src<C>();
  using V = typename C::value_type;
  const auto K = {compare, copy_ctor, move_ctor, allocation};
  const auto KI = {compare, copy_ctor, move_ctor, allocation, iter_inc, iter_deref, iter_cmp};
  for (int key : {25, 30}) {
    const char* tag = key == 25 ? " [new key]" : " [existing key]";
    char nm[128];
    auto name = [&](const char* what) {
      __builtin_snprintf(nm, sizeof nm, "%s%s", what, tag);
      return nm;
    };
    static V v1 = val<C>(25), v2 = val<C>(30);
    V& v = key == 25 ? v1 : v2;
    go<C>(cname, name("insert(const V&)"), G::strong, K, [&](C& c) { c.insert(v); });
    go<C>(cname, name("insert(V&&)"), G::strong, K, [&](C& c) {
      V t = v; // constructed inside the attempt: its copy may throw too
      c.insert(std::move(t));
    });
    go<C>(cname, name("insert(hint, const V&)"), G::strong, K, [&](C& c) { c.insert(c.begin(), v); });
    go<C>(cname, name("insert(end hint, const V&)"), G::strong, K, [&](C& c) { c.insert(c.end(), v); });
    if constexpr (is_map<C>) {
      go<C>(cname, name("emplace(int, int)"), G::strong, {compare, value_ctor, allocation},
            [&](C& c) { c.emplace(key, 7); });
      go<C>(cname, name("emplace_hint(hint, int, int)"), G::strong, {compare, value_ctor, allocation},
            [&](C& c) { c.emplace_hint(c.begin(), key, 7); });
    } else {
      go<C>(cname, name("emplace(int)"), G::strong, {compare, value_ctor, allocation},
            [&](C& c) { c.emplace(key); });
      go<C>(cname, name("emplace_hint(hint, int)"), G::strong, {compare, value_ctor, allocation},
            [&](C& c) { c.emplace_hint(c.begin(), key); });
    }
    if constexpr (is_map<C> && !is_multi<C>) {
      static const T kk[2] = {T(25), T(30)};
      const T& kref = kk[key == 25 ? 0 : 1];
      go<C>(cname, name("operator[](const key&)"), G::strong, {compare, copy_ctor, default_ctor, allocation},
            [&](C& c) { c[kref]; });
      go<C>(cname, name("try_emplace(const key&, int)"), G::strong, {compare, copy_ctor, value_ctor, allocation},
            [&](C& c) { c.try_emplace(kref, 9); });
      go<C>(cname, name("try_emplace(hint, const key&, int)"), G::strong,
            {compare, copy_ctor, value_ctor, allocation}, [&](C& c) { c.try_emplace(c.end(), kref, 9); });
      // insert_or_assign of an existing key assigns: not an insertion, basic guarantee.
      go<C>(cname, name("insert_or_assign(const key&, const T&)"), key == 25 ? G::strong : G::basic,
            {compare, copy_ctor, copy_assign, allocation}, [&](C& c) { c.insert_or_assign(kref, kk[0]); });
    }
  }
  // insert(node_type&&): the node comes from another container with an equal allocator.
  for (int key : {25, 30})
    for (Kind k : {compare})
      sweep(key == 25 ? "insert(nh) [new key]" : "insert(nh) [existing key]", k, [&] {
        C c = make<C>();
        C other;
        other.insert(val<C>(key));
        auto nh = other.extract(other.begin());
        snap before = snap::of(c);
        long live0 = st.live;
        bool threw = attempt([&] { c.insert(std::move(nh)); });
        if (threw) {
          EXH_EXPECT(snap::of(c) == before, "insert(nh) had an effect on the container although it threw");
          EXH_EXPECT(!nh.empty(), "insert(nh) emptied the node handle although the insertion threw");
          EXH_EXPECT(st.live == live0, "element objects created or destroyed");
          check_valid(c);
        }
        return threw;
      });
  go<C>(cname, "insert(input first, last)", G::basic, KI, [](C& c) {
    range<in_tag, V> r{src<C>(), src<C>() + 6};
    c.insert(r.begin(), r.end());
  });
  go<C>(cname, "insert(ra first, last)", G::basic, KI, [](C& c) {
    range<ra_tag, V> r{src<C>(), src<C>() + 6};
    c.insert(r.begin(), r.end());
  });
  go<C>(cname, "insert_range(fwd)", G::basic, KI, [](C& c) { c.insert_range(range<fwd_tag, V>{src<C>(), src<C>() + 6}); });
  go<C>(cname, "insert(il)", G::basic, K, [](C& c) { c.insert({src<C>()[0], src<C>()[2], src<C>()[3]}); });
  go<C>(cname, "erase(key)", G::basic, {compare}, [](C& c) { c.erase(key_of<C>(src<C>()[1])); });
  go<C>(cname, "extract(key)", G::basic, {compare}, [](C& c) { auto nh = c.extract(key_of<C>(src<C>()[1])); });
  go<C>(cname, "C(input first, last)", G::basic, KI, [](C&) {
    range<in_tag, V> r{src<C>(), src<C>() + 6};
    C x(r.begin(), r.end());
  });
  go<C>(cname, "C(from_range, ra)", G::basic, KI, [](C&) { C x(std::from_range, range<ra_tag, V>{src<C>(), src<C>() + 6}); });
  go<C>(cname, "C(il)", G::basic, K, [](C&) { C x{src<C>()[0], src<C>()[1], src<C>()[2], src<C>()[3]}; });
  go<C>(cname, "C(const C&)", G::basic, K, [](C& c) { C x(c); });
  static C big, small;
  big.clear();
  small.clear();
  for (int i = 0; i < 12; ++i) big.insert(val<C>(100 + i));
  small.insert(val<C>(7));
  go<C>(cname, "copy assignment from a larger container", G::basic, K, [](C& c) { c = big; });
  go<C>(cname, "copy assignment from a smaller container", G::basic, K, [](C& c) { c = small; });
  go<C>(cname, "operator=(il)", G::basic, K, [](C& c) { c = {src<C>()[0], src<C>()[3]}; });
}

int main() {
  run<Set>("set");
  run<MSet>("multiset");
  run<Map>("map");
  run<MMap>("multimap");
  return finish();
}
