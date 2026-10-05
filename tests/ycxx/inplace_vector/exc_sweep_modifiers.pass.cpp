// Exception-injection sweep over inplace_vector's constructors, modifiers and assignment
// members (support/exc_sequence.hpp; see vector/exc_sweep_modifiers for the accounting
// checks), plus capacity overflow and try_append_range.
//   [inplace.vector.modifiers]/3 (insert, insert_range, emplace, append_range): "If an
//     exception is thrown other than by the copy constructor, move constructor, assignment
//     operator, or move assignment operator of T or by any InputIterator operation, there are
//     no effects. Otherwise, if an exception is thrown, then size() >= n and elements in the
//     range begin() + [0, n) are not modified" (n: size() before append_range, otherwise
//     distance(begin, position)).
//   /7 (push_back, emplace_back): "If an exception is thrown, there are no effects on *this."
//   [inplace.vector.capacity]/4, /7: resize: "If an exception is thrown, there are no effects
//     on *this."
//   /5 and [inplace.vector.overview]: exceeding the capacity throws bad_alloc, which is not
//     thrown by T or an iterator, so there are no effects.
//   /14 (try_push_back, try_emplace_back): "If an exception is thrown, there are no effects on
//     *this"; /10-11: when full, no effects and nullopt.
// REQUIRES: exceptions
#include <inplace_vector>
#include <new>
#include "exc_sequence.hpp"

using namespace exh;
using namespace exh::seq;

template <class Elem>
struct IvPolicy {
  static constexpr const char* name = "inplace_vector";
  using E = Elem;
  using C = std::inplace_vector<E, 16>;
  using C2 = void;
  static constexpr int variants = 1;
  static constexpr bool ordered = true;
  static C make(int) {
    C c;
    for (int i = 0; i < 5; ++i) c.emplace_back(10 + i);
    return c;
  }
  static G guarantee(Pos p, Op o, Kind k) {
    bool t_op = k == copy_ctor || k == move_ctor || k == copy_assign || k == move_assign;
    bool it_op = k == iter_inc || k == iter_deref || k == iter_cmp;
    switch (o) {
    case Op::single:
      if (p == Pos::back) return G::strong;
      return t_op ? G::prefix : G::strong;
    case Op::multi:
      return t_op || it_op ? G::prefix : G::strong;
    case Op::resize:
      return G::strong;
    default:
      return G::basic;
    }
  }
};

using IV = std::inplace_vector<T, 8>;

static T* src() {
  static T s[6] = {T(50), T(51), T(52), T(53), T(54), T(55)};
  return s;
}

static IV five() {
  IV c;
  for (int i = 0; i < 5; ++i) c.emplace_back(10 + i);
  return c;
}

template <class F>
static void overflow(const char* name, F op, int fill = 5) {
  sweep(name, copy_ctor, [&] {
    IV c = five();
    while (int(c.size()) < fill) c.emplace_back(20);
    snap before = snap::of(c);
    long live0 = st.live;
    bool got_bad_alloc = false;
    bool threw = attempt([&] {
      try {
        op(c);
      } catch (const std::bad_alloc&) {
        got_bad_alloc = true;
      }
    });
    EXH_EXPECT(st.live - live0 == long(c.size()) - fill, "element accounting");
    if (got_bad_alloc) EXH_EXPECT(snap::of(c) == before, "capacity overflow (bad_alloc) had effects");
    if (!threw) EXH_EXPECT(got_bad_alloc, "capacity overflow did not throw bad_alloc");
    return threw;
  });
}

int main() {
  runner<IvPolicy<T>>::all();
  runner<IvPolicy<NT>>::all();
  (void)src();

  overflow("insert_range(mid, input range of 6) into 5/8", [](IV& c) { c.insert_range(c.begin() + 2, range<in_tag>{src(), src() + 6}); });
  overflow("append_range(input range of 6) into 5/8", [](IV& c) { c.append_range(range<in_tag>{src(), src() + 6}); });
  overflow("append_range(ra range of 6) into 5/8", [](IV& c) { c.append_range(range<ra_tag>{src(), src() + 6}); });
  overflow("insert(begin, 4, x) into 5/8", [](IV& c) { c.insert(c.begin(), 4, src()[0]); });
  overflow("insert(begin, input first, last) of 4 into 5/8", [](IV& c) {
    range<in_tag> r{src(), src() + 4};
    c.insert(c.begin(), r.begin(), r.end());
  });
  overflow("push_back into 8/8", [](IV& c) { c.push_back(src()[0]); }, 8);
  overflow("emplace(begin, int) into 8/8", [](IV& c) { c.emplace(c.begin(), 3); }, 8);
  overflow("resize(9)", [](IV& c) { c.resize(9, src()[0]); });

  for (int fill : {5, 8})
    sweep(fill == 5 ? "try_push_back into 5/8" : "try_push_back into 8/8", copy_ctor, [fill] {
      IV c = five();
      while (int(c.size()) < fill) c.emplace_back(20);
      snap before = snap::of(c);
      long live0 = st.live;
      bool engaged = false;
      bool threw = attempt([&] { engaged = c.try_push_back(src()[0]).has_value(); });
      EXH_EXPECT(st.live - live0 == long(c.size()) - fill, "element accounting");
      if (threw) EXH_EXPECT(snap::of(c) == before, "try_push_back had effects although it threw (/14)");
      if (!threw) EXH_EXPECT(engaged == (fill == 5) && int(c.size()) == (fill == 5 ? 6 : 8), "try_push_back result");
      return threw;
    });
  return finish();
}
