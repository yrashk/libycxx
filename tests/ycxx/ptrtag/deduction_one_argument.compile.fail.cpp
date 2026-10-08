// [ptrtag.pair.general]: the deduction guide pointer_tag_pair(Ptr*) -> pointer_tag_pair<Ptr*>
// has no constructor taking a pointer alone (STATUS "Draft issues noticed"): deduction picks the
// guide, and the initialization then finds no constructor.
// EXPECT-ERROR-GCC: no matching function for call to .std::(__y1::)?pointer_tag_pair<int\*, 2, unsigned int>::pointer_tag_pair\(int\*\)
// EXPECT-ERROR-CLANG: no matching conversion for functional-style cast from 'int \*' to 'std::pointer_tag_pair<int \*>'
#include <memory>

alignas(8) int x;
auto p = std::pointer_tag_pair(&x);
