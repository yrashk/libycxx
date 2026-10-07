// [ptrtag.pair.cons]/2, [ptrtag.pair.overalign]/1: "Constant When: Preconditions are met": a
// non-zero tag stored during constant evaluation.
// GAP: any P1-01 (6) neither GCC 16.2 nor Clang 23.1 can set bits of a pointer during constant evaluation (DECISIONS §9)
// FREESTANDING
#include <memory>

struct alignas(16) N {
  int v;
};
constexpr bool ce() {
  N n{};
  std::pointer_tag_pair<N*> a(&n, 5u);
  std::pointer_tag_pair<N*> z(nullptr, 3u);
  return a.pointer() == &n && a.tag() == 5u && z.tag() == 3u;
}
static_assert(ce());
