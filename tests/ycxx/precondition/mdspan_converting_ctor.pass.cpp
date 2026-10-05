// [mdspan.mdspan.cons]: mdspan(const mdspan<OtherElementType, OtherExtents, ...>&): "Hardened
// preconditions: For each rank index r of extents_type, static_extent(r) == dynamic_extent ||
// static_extent(r) == other.extent(r) is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <mdspan>
#include <cstddef>
#include "violation.hpp"

int main() {
  int a[4] = {};
  std::mdspan<int, std::dextents<std::size_t, 1>> d(a, 2);
  about_to_violate("mdspan_converting_ctor");
  std::mdspan<int, std::extents<std::size_t, 4>> s(d);
  keep(s);
  never_reached();
}
