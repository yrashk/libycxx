// Exception-injection sweep over deque's constructors, modifiers, capacity and assignment
// members (support/exc_sequence.hpp; see vector/exc_sweep_modifiers for the accounting checks).
// Guarantees checked on an exception:
//   [deque.modifiers]/3: "If an exception is thrown other than by the copy constructor, move
//     constructor, assignment operator, or move assignment operator of T, there are no effects.
//     If an exception is thrown while inserting a single element at either end, there are no
//     effects." Unlike [vector.modifiers]/2, InputIterator operations are NOT exempted: a source
//     iterator that throws during insert/insert_range/append_range/prepend_range leaves the
//     deque unchanged.
//   [container.reqmts]/66.1-66.2: single-element insert/emplace and push_*/emplace_* have no
//     effects (subject to /3 for the middle).
//   [deque.capacity]/6: shrink_to_fit has no effects if an exception is thrown (other than by
//     the move constructor of a non-Cpp17CopyInsertable T).
//   resize ([deque.capacity]/1-4), assignments, erase, constructors: basic guarantee.
#include <deque>
#include "exc_sequence.hpp"

using namespace exh;
using namespace exh::seq;

template <class Elem>
struct DequePolicy {
  static constexpr const char* name = "deque";
  using E = Elem;
  using C = std::deque<E, alloc<E>>;
  using C2 = std::deque<E, alloc<E, false>>;
  static constexpr int variants = 2;
  static constexpr bool ordered = true;
  static C make(int variant) {
    C c;
    if (variant == 1) // elements straddling a block boundary at the front
      for (int i = 0; i < 40; ++i) c.emplace_front(0);
    for (int i = 0; i < 5; ++i) c.emplace_back(10 + i);
    if (variant == 1) {
      for (int i = 0; i < 40; ++i) c.pop_front();
    }
    return c;
  }
  static G guarantee(Pos p, Op o, Kind k) {
    bool t_op = k == copy_ctor || k == move_ctor || k == copy_assign || k == move_assign;
    switch (o) {
    case Op::single:
      if (p != Pos::mid && p != Pos::late) return G::strong; // at either end
      return t_op ? G::basic : G::strong;
    case Op::multi:
      return t_op ? G::basic : G::strong;
    case Op::shrink:
      return G::strong;
    default:
      return G::basic;
    }
  }
};

int main() {
  runner<DequePolicy<T>>::all();
  runner<DequePolicy<NT>>::all();
  return finish();
}
