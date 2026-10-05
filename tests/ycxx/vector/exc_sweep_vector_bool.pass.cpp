// Exception-injection sweep over vector<bool>'s constructors, modifiers, capacity and
// assignment members (support/exc_sequence.hpp): the allocator's allocate and the source
// iterator's operations throw at their k-th call. Every allocated block must be deallocated
// exactly once with the size passed to allocate ([allocator.requirements.general]).
//   [vector.bool.pspc]/1: "Unless described below, all operations have the same requirements and
//     semantics as the primary vector template, except that operations dealing with the bool
//     value type map to bit values in the container storage".
//   Hence [vector.modifiers]/2: an exception thrown other than by T's copy/move
//     construction/assignment or by an InputIterator operation (here: allocate) has no
//     effects; [vector.capacity]/4, /16, /19: reserve and resize have no effects.
//   Iterator exceptions and assignments: basic guarantee.
// REQUIRES: exceptions
#include <vector>
#include "exc_sequence.hpp"

using namespace exh;
using namespace exh::seq;

struct VbPolicy {
  static constexpr const char* name = "vector<bool>";
  using E = bool;
  using C = std::vector<bool, alloc<bool>>;
  using C2 = std::vector<bool, alloc<bool, false>>;
  static constexpr int variants = 2;
  static constexpr bool ordered = true;
  static bool val(int i) { return i % 3 == 1; }
  static C make(int variant) {
    C c;
    // variant 0: 5 bits; variant 1: 70 bits (more than one word), spare capacity
    int n = variant == 0 ? 5 : 70;
    if (variant == 1) c.reserve(200);
    for (int i = 0; i < n; ++i) c.push_back(i % 2 == 0);
    return c;
  }
  static G guarantee(Pos, Op o, Kind k) {
    bool it_op = k == iter_inc || k == iter_deref || k == iter_cmp;
    switch (o) {
    case Op::single:
      return G::strong;
    case Op::multi:
      return it_op ? G::basic : G::strong;
    case Op::resize:
    case Op::reserve:
    case Op::shrink:
      return G::strong;
    default:
      return G::basic;
    }
  }
};

int main() {
  runner<VbPolicy>::all();
  return finish();
}
