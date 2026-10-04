// Exception-injection sweep over the container adaptors stack, queue and priority_queue over
// deque/vector with exh::alloc: element construction/assignment, the allocator, source
// iterators and priority_queue's comparison throw at their k-th call, for every k.
//   [stack.mod], [queue.mod]: push(x) is c.push_back(x), emplace is c.emplace_back(...),
//     push_range is c.append_range(rg) (or insert): so push/emplace have no effects when they
//     throw ([container.reqmts]/66.2, [deque.modifiers]/3); queue::pop/stack::pop do not throw
//     (/66.3).
//   [priqueue.members]: push(x) is "c.push_back(x); push_heap(c.begin(), c.end(), comp);",
//     pop() is "pop_heap(c.begin(), c.end(), comp); c.pop_back();": no guarantee beyond the
//     basic one when comp throws, but every element object stays accounted for (the element
//     count of c equals the live objects it owns) and nothing leaks.
//   Constructors from ranges (make_heap) and push_range: basic guarantee.
#include <deque>
#include <queue>
#include <stack>
#include <vector>
#include "exc_harness.hpp"

using namespace exh;
using D = std::deque<T, alloc<T>>;
using Vt = std::vector<T, alloc<T>>;
using St = std::stack<T, D>;
using Qu = std::queue<T, D>;
using PQ = std::priority_queue<T, Vt, exh::less>;

static T* src() {
  static T s[6] = {T(5), T(50), T(35), T(15), T(45), T(25)};
  return s;
}

template <class A>
const auto& under(const A& a) {
  struct X : A {
    static const auto& get(const A& a) { return a.*(&X::c); }
  };
  return X::get(a);
}

template <class A>
A make() {
  A a;
  for (int v : {10, 40, 20, 30, 50}) a.push(T(v));
  return a;
}

template <class A, class Op>
void go(const char* name, bool strong, std::initializer_list<Kind> kinds, Op op) {
  for (Kind k : kinds)
    sweep(name, k, [&] {
      A a = make<A>();
      snap before = snap::of(under(a));
      long live0 = st.live;
      long n0 = long(a.size());
      bool threw = attempt([&] { op(a); });
      EXH_EXPECT(st.live - live0 == long(a.size()) - n0, "live element objects do not match the size");
      if (threw && strong) EXH_EXPECT(snap::of(under(a)) == before, "push/emplace had an effect although it threw");
      if constexpr (std::is_same_v<A, PQ>) {
        if (!threw) {
          // the heap still works: popping yields a non-increasing sequence
          int prev = 1 << 30;
          bool ok = true;
          while (!a.empty()) {
            ok = ok && a.top().v <= prev;
            prev = a.top().v;
            a.pop();
          }
          EXH_EXPECT(ok, "priority_queue order broken");
        }
      }
      return threw;
    });
}

int main() {
  (void)src();
  const auto K = {copy_ctor, move_ctor, copy_assign, move_assign, allocation};
  const auto KI = {copy_ctor, move_ctor, allocation, iter_inc, iter_deref, iter_cmp};
  go<St>("stack::push(const T&)", true, K, [](St& s) { s.push(src()[0]); });
  go<St>("stack::emplace(int)", true, {value_ctor, allocation}, [](St& s) { s.emplace(7); });
  go<St>("stack::push_range(input)", false, KI, [](St& s) { s.push_range(range<in_tag>{src(), src() + 6}); });
  go<St>("stack(from_range, ra)", false, KI, [](St&) { St x(std::from_range, range<ra_tag>{src(), src() + 6}); });
  go<St>("stack(input first, last)", false, KI, [](St&) {
    range<in_tag> r{src(), src() + 6};
    St x(r.begin(), r.end());
  });
  go<Qu>("queue::push(const T&)", true, K, [](Qu& q) { q.push(src()[0]); });
  go<Qu>("queue::emplace(int)", true, {value_ctor, allocation}, [](Qu& q) { q.emplace(7); });
  go<Qu>("queue::push_range(fwd)", false, KI, [](Qu& q) { q.push_range(range<fwd_tag>{src(), src() + 6}); });
  go<Qu>("queue copy", false, K, [](Qu& q) { Qu x(q); });

  const auto KC = {compare, copy_ctor, move_ctor, copy_assign, move_assign, allocation};
  go<PQ>("priority_queue::push(const T&)", false, KC, [](PQ& p) { p.push(src()[1]); });
  go<PQ>("priority_queue::emplace(int)", false, {compare, value_ctor, move_assign, allocation},
         [](PQ& p) { p.emplace(60); });
  go<PQ>("priority_queue::pop()", false, {compare, move_ctor, move_assign}, [](PQ& p) { p.pop(); });
  go<PQ>("priority_queue::push_range(input)", false,
         {compare, copy_ctor, move_ctor, move_assign, allocation, iter_inc, iter_deref, iter_cmp},
         [](PQ& p) { p.push_range(range<in_tag>{src(), src() + 6}); });
  go<PQ>("priority_queue(input first, last)", false,
         {compare, copy_ctor, move_ctor, move_assign, allocation, iter_inc, iter_deref, iter_cmp}, [](PQ&) {
           range<in_tag> r{src(), src() + 6};
           PQ x(r.begin(), r.end());
         });
  go<PQ>("priority_queue(from_range, ra)", false,
         {compare, copy_ctor, move_ctor, move_assign, allocation, iter_inc, iter_deref, iter_cmp},
         [](PQ&) { PQ x(std::from_range, range<ra_tag>{src(), src() + 6}); });
  go<PQ>("priority_queue(comp, const Vt&)", false, KC, [](PQ&) {
    Vt w;
    for (int i : {3, 9, 1, 7, 5}) w.emplace_back(i);
    PQ x(exh::less{}, w);
  });
  return finish();
}
