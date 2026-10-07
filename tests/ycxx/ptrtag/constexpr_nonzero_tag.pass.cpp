// [ptrtag.pair.cons]/2 and [ptrtag.pair.overalign]/1: "Constant When: Preconditions are met." --
// so a non-zero tag that fits in bits_requested can be stored during constant evaluation, and
// pointer() and tag() give it back ([ptrtag.pair.cons]/4, [ptrtag.pair.overalign]/3).
// XFAIL: any  compiler-blocked: neither GCC 16.2 nor Clang 23.1 can set bits of a pointer during constant evaluation (no pointer-tagging builtin; reinterpret_cast, bit_cast of pointers and out-of-object arithmetic are rejected), DECISIONS §9, STATUS "Known compiler gaps"
#include <memory>
#include "check.hpp"

struct alignas(16) Node {
  int v;
};
enum class Color : unsigned { none, red, green };

constexpr bool run() {
  Node n{};
  std::pointer_tag_pair<Node*> a(&n, 5u);
  if (a.pointer() != &n || a.tag() != 5u)
    return false;
  std::pointer_tag_pair<Node*, 2, Color> c(&n, Color::green);
  if (c.pointer() != &n || c.tag() != Color::green)
    return false;
  std::pointer_tag_pair<Node*> z(nullptr, 3u);
  if (z.pointer() != nullptr || z.tag() != 3u)
    return false;
  alignas(64) char buf[64] = {};
  auto o = std::pointer_tag_pair<char*, 6>::from_overaligned<64>(buf, 63u);
  if (o.pointer() != buf || o.tag() != 63u)
    return false;
  // Ordered by tag when the pointers are equal.
  return std::pointer_tag_pair<Node*>(&n, 1u) < std::pointer_tag_pair<Node*>(&n, 2u);
}
static_assert(run());

int main() {
  CHECK(run());
  return 0;
}
