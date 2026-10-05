// [depr.move.iter.elem] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
// COUNTERPART: libcxx:iterators/predef.iterators/move.iterators/move.iter.ops/move.iter.op.ref/deprecated.verify.cpp
#include <iterator>
#include <cstddef>

int main() {
  int a[1]{}; std::move_iterator<int*> m(a); int* p = m.operator->(); (void)p;
}
