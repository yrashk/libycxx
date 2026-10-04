// Exception-injection sweep over list's constructors, modifiers and assignment members
// (support/exc_sequence.hpp; see vector/exc_sweep_modifiers for the accounting checks), and
// over the list operations that take user predicates and comparators.
//   [list.modifiers]/2 (every insert, emplace, push and prepend/append_range): "If an exception
//     is thrown, there are no effects."
//   resize ([list.capacity]), assignments, constructors: basic guarantee.
//   [list.ops]/17, /24: remove_if/unique throw only what the predicate throws: basic, but no
//     element may be lost or duplicated: the remaining elements are a subsequence of the
//     original, and the erased ones are destroyed.
//   [list.ops]/30: merge: "If an exception is thrown other than by a comparison, there are no
//     effects." "No elements are copied": on a comparison exception every element is still in
//     exactly one of the two lists.
//   [list.ops]/33: sort: "If an exception is thrown, the order of the elements in *this is
//     unspecified." The elements themselves stay (sort does not copy, construct or destroy).
#include <list>
#include "exc_sequence.hpp"

using namespace exh;
using namespace exh::seq;

template <class Elem>
struct ListPolicy {
  static constexpr const char* name = "list";
  using E = Elem;
  using C = std::list<E, alloc<E>>;
  using C2 = std::list<E, alloc<E, false>>;
  static constexpr int variants = 1;
  static constexpr bool ordered = true;
  static C make(int) {
    C c;
    for (int i = 0; i < 5; ++i) c.emplace_back(10 + i);
    return c;
  }
  static G guarantee(Pos, Op o, Kind) {
    return o == Op::single || o == Op::multi ? G::strong : G::basic;
  }
};

using L = std::list<T, alloc<T>>;

static L mixed() {
  L c;
  for (int v : {5, 3, 3, 8, 1, 1, 1, 9, 2, 6, 6, 4})
    c.emplace_back(v);
  return c;
}

static bool is_subsequence(const snap& sub, const snap& full) {
  int j = 0;
  for (int i = 0; i < sub.n; ++i) {
    while (j < full.n && full.v[j] != sub.v[i]) ++j;
    if (j == full.n) return false;
    ++j;
  }
  return true;
}

int main() {
  runner<ListPolicy<T>>::all();
  runner<ListPolicy<NT>>::all();

  sweep("list::remove_if(throwing pred)", pred, [] {
    L c = mixed();
    snap before = snap::of(c);
    long live0 = st.live;
    bool threw = attempt([&] { c.remove_if(pred_odd{}); });
    EXH_EXPECT(st.live - live0 == long(c.size()) - before.n, "erased elements not destroyed exactly once");
    EXH_EXPECT(is_subsequence(snap::of(c), before), "remaining elements are not a subsequence of the original");
    return threw;
  });
  sweep("list::unique(throwing pred)", compare, [] {
    L c = mixed();
    snap before = snap::of(c);
    long live0 = st.live;
    bool threw = attempt([&] { c.unique(equal{}); });
    EXH_EXPECT(st.live - live0 == long(c.size()) - before.n, "erased elements not destroyed exactly once");
    EXH_EXPECT(is_subsequence(snap::of(c), before), "remaining elements are not a subsequence of the original");
    return threw;
  });
  sweep("list::sort(throwing comp)", compare, [] {
    L c = mixed();
    snap before = snap::of(c);
    long live0 = st.live;
    bool threw = attempt([&] { c.sort(less{}); });
    EXH_EXPECT(st.live == live0, "sort constructed or destroyed elements");
    EXH_EXPECT(snap::of(c).sorted() == before.sorted(), "sort lost or duplicated elements");
    if (!threw) EXH_EXPECT(snap::of(c).is_sorted(false), "not sorted");
    return threw;
  });
  sweep("list::merge(throwing comp)", compare, [] {
    L a, b;
    for (int v : {1, 4, 6, 9, 12}) a.emplace_back(v);
    for (int v : {2, 3, 7, 10, 11, 13}) b.emplace_back(v);
    snap all = snap::of(a);
    for (auto& x : b) all.v[all.n++] = x.v;
    long live0 = st.live;
    bool threw = attempt([&] { a.merge(b, less{}); });
    EXH_EXPECT(st.live == live0, "merge constructed or destroyed elements");
    EXH_EXPECT(long(a.size() + b.size()) == all.n, "elements lost by merge");
    snap both = snap::of(a);
    for (auto& x : b) both.v[both.n++] = x.v;
    EXH_EXPECT(both.sorted() == all.sorted(), "merge lost or duplicated elements");
    return threw;
  });
  return finish();
}
