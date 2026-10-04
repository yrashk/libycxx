// Exception-injection sweep over hive's constructors, modifiers and assignment members
// (support/exc_sequence.hpp; see vector/exc_sweep_modifiers for the accounting checks; the
// order of a hive's elements is unspecified, so contents are compared as multisets).
//   [hive.modifiers]/2 (emplace, emplace_hint, and insert(x)/insert(hint, x) by /6): "If an
//     exception is thrown, there are no effects."
//   Range and n-copies insertion, assignments, constructors, reserve, shrink_to_fit
//   ([hive.capacity]/9: "Otherwise if an exception is thrown, the effects are unspecified"):
//   basic guarantee: every element object accounted for, every block deallocated.
// Two initial states: five contiguous elements, and five elements left after erasing others
// (erased slots to be reused).
#include <hive>
#include "exc_sequence.hpp"

using namespace exh;
using namespace exh::seq;

template <class Elem>
struct HivePolicy {
  static constexpr const char* name = "hive";
  using E = Elem;
  using C = std::hive<E, alloc<E>>;
  using C2 = std::hive<E, alloc<E, false>>;
  static constexpr int variants = 2;
  static constexpr bool ordered = false;
  static C make(int variant) {
    C c;
    if (variant == 0) {
      for (int i = 0; i < 5; ++i) c.emplace(10 + i);
    } else {
      for (int i = 0; i < 9; ++i) c.emplace(i % 2 ? 10 + i / 2 : 0);
      for (auto it = c.begin(); it != c.end();)
        if (it->v == 0)
          it = c.erase(it);
        else
          ++it;
      c.emplace(14);
    }
    return c;
  }
  static G guarantee(Pos, Op o, Kind) { return o == Op::single ? G::strong : G::basic; }
};

int main() {
  runner<HivePolicy<T>>::all();
  runner<HivePolicy<NT>>::all();
  return finish();
}
