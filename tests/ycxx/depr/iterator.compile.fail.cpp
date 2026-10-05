// [depr.iterator] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
// COUNTERPART: libcxx:iterators/iterator.primitives/iterator.basic/deprecated.verify.cpp
#include <iterator>
#include <cstddef>

int main() {
  struct I : std::iterator<std::input_iterator_tag, int> {}; I i; (void)i;
}
