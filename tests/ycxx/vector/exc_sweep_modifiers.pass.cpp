// Exception-injection sweep over vector's constructors, modifiers, capacity and assignment
// members (support/exc_sequence.hpp): the k-th element copy/move construction or assignment,
// default or converting construction, allocator allocate, or source-iterator operation throws,
// for every k until the operation completes. After every run each element object constructed
// has been destroyed exactly once and every allocated block deallocated exactly once with its
// size ([res.on.exception.handling]/1; [allocator.requirements.general]: deallocate(p, n) with
// the n passed to allocate; [container.reqmts]: the container's destructor destroys its
// elements and deallocates its storage). Guarantees checked on an exception:
//   [vector.modifiers]/2: "If an exception is thrown other than by the copy constructor, move
//     constructor, assignment operator, or move assignment operator of T or by any
//     InputIterator operation, there are no effects. If an exception is thrown while inserting
//     a single element at the end and T is Cpp17CopyInsertable or is_nothrow_move_constructible_v<T>
//     is true, there are no effects."  (Element-type T here is Cpp17CopyInsertable.)
//   [vector.capacity]/4 (reserve), /9 (shrink_to_fit), /16 (resize(sz)): no effects unless
//     thrown by the move constructor of a non-Cpp17CopyInsertable type; /19 (resize(sz, c)):
//     "If an exception is thrown, there are no effects."
//   Otherwise (assignments, erase, constructors): the basic guarantee only.
// Two element types: every special member may throw (exh::T), and noexcept moves (exh::NT),
// which take different relocation paths; two capacities: none spare and plenty spare.
// REQUIRES: exceptions
#include <vector>
#include "exc_sequence.hpp"

using namespace exh;
using namespace exh::seq;

template <class Elem>
struct VecPolicy {
  static constexpr const char* name = "vector";
  using E = Elem;
  using C = std::vector<E, alloc<E>>;
  using C2 = std::vector<E, alloc<E, false>>;
  static constexpr int variants = 2;
  static constexpr bool ordered = true;
  static C make(int variant) {
    C c;
    c.reserve(variant == 0 ? 5 : 32);
    for (int i = 0; i < 5; ++i) c.emplace_back(10 + i);
    return c;
  }
  static G guarantee(Pos p, Op o, Kind k) {
    bool t_op = k == copy_ctor || k == move_ctor || k == copy_assign || k == move_assign;
    bool it_op = k == iter_inc || k == iter_deref || k == iter_cmp;
    switch (o) {
    case Op::single:
      if (p == Pos::back || p == Pos::end) return G::strong;
      return t_op ? G::basic : G::strong;
    case Op::multi:
      return t_op || it_op ? G::basic : G::strong;
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
  runner<VecPolicy<T>>::all();
  runner<VecPolicy<NT>>::all();
  return finish();
}
