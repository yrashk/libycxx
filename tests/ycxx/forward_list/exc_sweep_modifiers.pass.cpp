// Exception-injection sweep over forward_list's constructors, modifiers and assignment members
// (support/exc_sequence.hpp; see vector/exc_sweep_modifiers for the accounting checks), and
// over the operations that take user predicates and comparators.
//   [forward.list.modifiers]/1: "If an exception is thrown by any of these member functions
//     there is no effect on the container." This subclause includes insert_after, emplace_after,
//     insert_range_after, push_front, emplace_front, prepend_range AND resize
//     ([forward.list.modifiers] declares resize(sz) and resize(sz, c)).
//   assignments, constructors: basic guarantee.
//   [forward.list.ops]/15, /22: remove_if/unique: the remaining elements are a subsequence of
//     the original and erased ones destroyed; /28 merge: "If an exception is thrown other than
//     by a comparison, there are no effects", "No elements are copied"; /29 sort: "If an
//     exception is thrown, the order of the elements in *this is unspecified."
// REQUIRES: exceptions
#include <forward_list>
#include "exc_sequence.hpp"

using namespace exh;
using namespace exh::seq;

template <class Elem>
struct FlPolicy {
  static constexpr const char* name = "forward_list";
  using E = Elem;
  using C = std::forward_list<E, alloc<E>>;
  using C2 = void;
  static constexpr int variants = 1;
  static constexpr bool ordered = true;
  static C make(int) {
    C c;
    for (int i = 4; i >= 0; --i) c.emplace_front(10 + i);
    return c;
  }
  static G guarantee(Pos, Op o, Kind) {
    return o == Op::single || o == Op::multi || o == Op::resize ? G::strong : G::basic;
  }
};

using FL = std::forward_list<T, alloc<T>>;

static FL mixed() {
  FL c;
  for (int v : {4, 6, 6, 2, 9, 1, 1, 1, 8, 3, 3, 5})
    c.emplace_front(v);
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
  runner<FlPolicy<T>>::all();
  runner<FlPolicy<NT>>::all();

  sweep("forward_list::remove_if(throwing pred)", pred, [] {
    FL c = mixed();
    snap before = snap::of(c);
    long live0 = st.live;
    bool threw = attempt([&] { c.remove_if(pred_odd{}); });
    EXH_EXPECT(st.live - live0 == count(c) - before.n, "erased elements not destroyed exactly once");
    EXH_EXPECT(is_subsequence(snap::of(c), before), "remaining elements are not a subsequence of the original");
    return threw;
  });
  sweep("forward_list::unique(throwing pred)", compare, [] {
    FL c = mixed();
    snap before = snap::of(c);
    long live0 = st.live;
    bool threw = attempt([&] { c.unique(equal{}); });
    EXH_EXPECT(st.live - live0 == count(c) - before.n, "erased elements not destroyed exactly once");
    EXH_EXPECT(is_subsequence(snap::of(c), before), "remaining elements are not a subsequence of the original");
    return threw;
  });
  sweep("forward_list::sort(throwing comp)", compare, [] {
    FL c = mixed();
    snap before = snap::of(c);
    long live0 = st.live;
    bool threw = attempt([&] { c.sort(less{}); });
    EXH_EXPECT(st.live == live0, "sort constructed or destroyed elements");
    EXH_EXPECT(snap::of(c).sorted() == before.sorted(), "sort lost or duplicated elements");
    return threw;
  });
  sweep("forward_list::merge(throwing comp)", compare, [] {
    FL a, b;
    for (int v : {12, 9, 6, 4, 1}) a.emplace_front(v);
    for (int v : {13, 11, 10, 7, 3, 2}) b.emplace_front(v);
    snap all = snap::of(a);
    for (auto& x : b) all.v[all.n++] = x.v;
    long live0 = st.live;
    bool threw = attempt([&] { a.merge(b, less{}); });
    EXH_EXPECT(st.live == live0, "merge constructed or destroyed elements");
    snap both = snap::of(a);
    for (auto& x : b) both.v[both.n++] = x.v;
    EXH_EXPECT(both.sorted() == all.sorted(), "merge lost or duplicated elements");
    return threw;
  });
  return finish();
}
