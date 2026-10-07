// [ptrtag.pair.general]: the default constructor, the constructors, from_overaligned, pointer(),
// tag(), swap, the comparisons and get are constexpr; [ptrtag.pair.cons]/2 and
// [ptrtag.pair.overalign]/1: "Constant When: Preconditions are met."
// This test covers the tag 0 (TagT()), which libycxx can represent during constant evaluation;
// constexpr_nonzero_tag.pass.cpp covers the other tags (compiler-blocked, DECISIONS §9).
#include <compare>
#include <cstddef>
#include <memory>
#include <tuple>
#include "check.hpp"

struct alignas(16) Node {
  int v;
};
struct alignas(8) Base {
  long b;
};
struct Derived : Base {};
enum class Color : unsigned { none, red };

Node global_nodes[3];
Derived global_derived;

constexpr bool run() {
  using PT = std::pointer_tag_pair<Node*>;
  PT d;
  if (d.pointer() != nullptr || d.tag() != 0u)
    return false;
  Node local{};
  PT a(&local, 0u);
  if (a.pointer() != &local || a.tag() != 0u)
    return false;
  PT n(nullptr, 0u);
  if (n.pointer() != nullptr || !(n == d))
    return false;
  // Copy, assignment, swap.
  PT b = a;
  if (!(b == a))
    return false;
  b = n;
  b.swap(a);
  if (b.pointer() != &local || a.pointer() != nullptr)
    return false;
  // Enumeration tag, base pointers.
  Derived der{};
  std::pointer_tag_pair<Base*, 3, Color> pb(&der, Color::none);
  if (pb.pointer() != static_cast<Base*>(&der) || pb.tag() != Color::none)
    return false;
  // from_overaligned.
  alignas(64) char buf[64] = {};
  auto o = std::pointer_tag_pair<char*, 6>::from_overaligned<64>(buf, 0u);
  if (o.pointer() != buf || o.tag() != 0u)
    return false;
  auto on = std::pointer_tag_pair<char*, 6>::from_overaligned<64>(static_cast<char*>(nullptr), 0u);
  if (on.pointer() != nullptr)
    return false;
  // get and structured bindings.
  auto [p, t] = b;
  if (p != &local || t != 0u || std::get<0>(b) != &local || std::get<1>(b) != 0u)
    return false;
  return true;
}
static_assert(run());

// Comparisons of pointers into one array are constant expressions.
constexpr bool compare() {
  using PT = std::pointer_tag_pair<Node*>;
  PT x(&global_nodes[0], 0u), y(&global_nodes[2], 0u);
  return (x <=> y) == std::strong_ordering::less && x < y && x != y && x == PT(&global_nodes[0], 0u);
}
static_assert(compare());

// /2-3.2: a tag wider than bits_requested violates the precondition: not a constant expression.
template <unsigned T>
concept constant_tag =
    requires { typename std::integral_constant<int, (std::pointer_tag_pair<int*, 2>(nullptr, T).pointer(), 0)>; };
static_assert(constant_tag<0>);
static_assert(!constant_tag<4>);

// A constant-initialized object is usable at run time.
constinit std::pointer_tag_pair<Node*> g(&global_nodes[1], 0u);
constexpr std::pointer_tag_pair<Base*, 3> gb(&global_derived, 0u);
static_assert(gb.pointer() == static_cast<Base*>(&global_derived));

int main() {
  CHECK(run());
  CHECK(g.pointer() == &global_nodes[1] && g.tag() == 0u);
  CHECK(g.tagged_pointer() == static_cast<void*>(&global_nodes[1]));
  g = std::pointer_tag_pair<Node*>(&global_nodes[2], 9u);
  CHECK(g.pointer() == &global_nodes[2] && g.tag() == 9u);
  CHECK(gb.pointer() == static_cast<Base*>(&global_derived) && gb.tag() == 0u);
  return 0;
}
