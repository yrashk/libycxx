// Exception-injection sweep over flat_map, flat_multimap, flat_set and flat_multiset adapting
// vector<T, exh::alloc<T>>: the comparison object, the element type's constructors and
// assignments, the allocator and source iterators throw at their k-th call, for every k until
// the operation completes.
//   [flat.map.overview]/6, [flat.multimap.overview]/6, [flat.set.overview]/6,
//   [flat.multiset.overview]/6: "If any member function in [flat.map.defn] exits via an
//     exception the invariants of the object argument are restored." The invariants (/5):
//     "it contains the same number of keys and values; the keys are sorted with respect to the
//     comparison object; and the value at offset off within the value container is the value
//     associated with the key at offset off within the key container" (unique keys for
//     flat_map/flat_set by /1).
//   [flat.map.erasure]/5: erase_if exiting via an exception leaves c "in a valid but
//     unspecified state" ("c still meets its invariants").
// Every element here is inserted with mapped value key + 1 (or a default-constructed 0 by
// operator[]), so a broken key/value association is visible. After every run all element
// objects are destroyed exactly once and every block deallocated exactly once with its size.
// Whether a single-element insertion that throws must also have no effect
// ([associative.reqmts.except]/2 via [flat.map.overview]/2) is not asserted; /6's note says the
// container can be emptied.
// REQUIRES: exceptions
#include <flat_map>
#include <flat_set>
#include <vector>
#include "exc_harness.hpp"

using namespace exh;
using V = std::vector<T, alloc<T>>;
using FM = std::flat_map<T, T, exh::less, V, V>;
using FMM = std::flat_multimap<T, T, exh::less, V, V>;
using FS = std::flat_set<T, exh::less, V>;
using FMS = std::flat_multiset<T, exh::less, V>;

template <class C>
constexpr bool is_map = requires { typename C::mapped_type; };
template <class C>
constexpr bool is_multi = std::is_same_v<C, FMM> || std::is_same_v<C, FMS>;

template <class C>
typename C::value_type val(int k) {
  if constexpr (is_map<C>)
    return {T(k), T(k + 1)};
  else
    return T(k);
}

template <class C>
C make() {
  C c;
  for (int k : {10, 20, 30, 40, 50}) c.insert(val<C>(k));
  if constexpr (is_multi<C>) c.insert(val<C>(30));
  return c;
}

template <class C>
long objects(const C& c) {
  if constexpr (is_map<C>)
    return long(c.keys().size() + c.values().size());
  else
    return long(c.size());
}

template <class C>
void check_invariants(const C& c) {
  if constexpr (is_map<C>) {
    const auto& ks = c.keys();
    const auto& vs = c.values();
    EXH_EXPECT(ks.size() == vs.size(), "(5.1) different numbers of keys and values");
    bool sorted = true, assoc = true;
    for (std::size_t i = 0; i < ks.size(); ++i) {
      if (i && (is_multi<C> ? ks[i].v < ks[i - 1].v : !(ks[i - 1].v < ks[i].v))) sorted = false;
      if (i < vs.size() && vs[i].v != ks[i].v + 1 && vs[i].v != 0) assoc = false;
    }
    EXH_EXPECT(sorted, "(5.2) keys not sorted (or not unique) after an exception");
    EXH_EXPECT(assoc, "(5.3) a value is not associated with its key after an exception");
  } else {
    bool sorted = true;
    T const* prev = nullptr;
    for (auto& x : c) {
      if (prev && (is_multi<C> ? x.v < prev->v : !(prev->v < x.v))) sorted = false;
      prev = &x;
    }
    EXH_EXPECT(sorted, "keys not sorted (or not unique) after an exception");
  }
}

long lost_single = 0;

template <class C, class Op>
void go(const char* cname, const char* opname, bool single, std::initializer_list<Kind> kinds, Op op) {
  static char label[160];
  __builtin_snprintf(label, sizeof label, "%s: %s", cname, opname);
  for (Kind k : kinds)
    sweep(label, k, [&] {
      C c = make<C>();
      snap before = snap::of(c);
      long live0 = st.live;
      long o0 = objects(c);
      bool threw = attempt([&] { op(c); });
      EXH_EXPECT(st.live - live0 == objects(c) - o0, "live element objects do not match the containers' sizes");
      if (threw) {
        check_invariants(c);
        if (single && !(snap::of(c) == before)) ++lost_single;
      }
      return threw;
    });
}

template <class C>
typename C::value_type* src() {
  static typename C::value_type s[6] = {val<C>(5), val<C>(30), val<C>(35), val<C>(55), val<C>(15), val<C>(35)};
  return s;
}

template <class C>
void run(const char* cname) {
  using VT = typename C::value_type;
  (void)src<C>();
  const auto K = {compare, copy_ctor, move_ctor, copy_assign, move_assign, allocation};
  const auto KI = {compare, copy_ctor, move_ctor, copy_assign, move_assign, allocation, iter_inc, iter_deref, iter_cmp};
  static VT v25 = val<C>(25), v30 = val<C>(30);
  for (VT* v : {&v25, &v30}) {
    go<C>(cname, "insert(const V&)", true, K, [v](C& c) { c.insert(*v); });
    go<C>(cname, "insert(hint, const V&)", true, K, [v](C& c) { c.insert(c.begin(), *v); });
    if constexpr (is_map<C>) {
      int key = v->first.v;
      go<C>(cname, "emplace(int, int)", true, K, [key](C& c) { c.emplace(key, key + 1); });
    } else {
      int key = v->v;
      go<C>(cname, "emplace(int)", true, K, [key](C& c) { c.emplace(key); });
    }
    if constexpr (is_map<C> && !is_multi<C>) {
      static const T k25(25), k30(30);
      const T* kp = v == &v25 ? &k25 : &k30;
      go<C>(cname, "operator[](const key&)", true, {compare, copy_ctor, move_ctor, default_ctor, move_assign, allocation},
            [kp](C& c) { c[*kp]; });
      go<C>(cname, "try_emplace(const key&, int)", true, {compare, copy_ctor, move_ctor, value_ctor, move_assign, allocation},
            [kp](C& c) { c.try_emplace(*kp, kp->v + 1); });
      go<C>(cname, "insert_or_assign(const key&, int)", true, K, [kp](C& c) { c.insert_or_assign(*kp, T(kp->v + 1)); });
    }
  }
  go<C>(cname, "insert(input first, last)", false, KI, [](C& c) {
    range<in_tag, VT> r{src<C>(), src<C>() + 6};
    c.insert(r.begin(), r.end());
  });
  go<C>(cname, "insert(ra first, last)", false, KI, [](C& c) {
    range<ra_tag, VT> r{src<C>(), src<C>() + 6};
    c.insert(r.begin(), r.end());
  });
  go<C>(cname, "insert_range(fwd)", false, KI, [](C& c) { c.insert_range(range<fwd_tag, VT>{src<C>(), src<C>() + 6}); });
  go<C>(cname, "insert(il)", false, K, [](C& c) { c.insert({src<C>()[0], src<C>()[2], src<C>()[3]}); });
  go<C>(cname, "erase(key)", false, K, [](C& c) {
    if constexpr (is_map<C>)
      c.erase(src<C>()[1].first);
    else
      c.erase(src<C>()[1]);
  });
  go<C>(cname, "erase(iterator)", false, K, [](C& c) { c.erase(c.begin() + 1); });
  go<C>(cname, "erase_if(throwing pred)", false, {pred, move_assign}, [](C& c) {
    if constexpr (is_map<C>)
      erase_if(c, [](const auto& e) { return pred_odd{}(e.first) || e.first.v == 20; });
    else
      erase_if(c, [](const T& e) { return pred_odd{}(e) || e.v == 20; });
  });
  go<C>(cname, "C(input first, last)", false, KI, [](C&) {
    range<in_tag, VT> r{src<C>(), src<C>() + 6};
    C x(r.begin(), r.end());
    check_invariants(x);
  });
  go<C>(cname, "C(from_range, ra)", false, KI, [](C&) { C x(std::from_range, range<ra_tag, VT>{src<C>(), src<C>() + 6}); });
  go<C>(cname, "C(const C&)", false, K, [](C& c) { C x(c); });
  static C big;
  big.clear();
  for (int i = 0; i < 12; ++i) big.insert(val<C>(100 + i));
  go<C>(cname, "copy assignment from a larger container", false, K, [](C& c) { c = big; });
  go<C>(cname, "operator=(il)", false, K, [](C& c) { c = {src<C>()[0], src<C>()[3], src<C>()[2]}; });
  if constexpr (is_map<C>) {
    go<C>(cname, "replace(keys, values)", false, K, [](C& c) {
      V ks, vs;
      ks.emplace_back(1);
      ks.emplace_back(3);
      vs.emplace_back(2);
      vs.emplace_back(4);
      c.replace(std::move(ks), std::move(vs));
    });
    go<C>(cname, "C(key_cont, mapped_cont) (sorts)", false, K, [](C&) {
      V ks, vs;
      for (int k : {50, 10, 40, 20, 30}) {
        ks.emplace_back(k);
        vs.emplace_back(k + 1);
      }
      C x(std::move(ks), std::move(vs));
      check_invariants(x);
    });
  } else {
    go<C>(cname, "C(cont) (sorts)", false, K, [](C&) {
      V ks;
      for (int k : {50, 10, 40, 20, 30, 20}) ks.emplace_back(k);
      C x(std::move(ks));
      check_invariants(x);
    });
  }
}

int main() {
  run<FM>("flat_map");
  run<FMM>("flat_multimap");
  run<FS>("flat_set");
  run<FMS>("flat_multiset");
  dprintf(1, "single-element insertions that changed the container while throwing: %ld\n", lost_single);
  return finish();
}
