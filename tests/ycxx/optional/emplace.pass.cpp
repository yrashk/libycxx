// [optional.assign]/29-40: emplace destroys any existing value then direct-non-list-initializes;
// returns a reference to the new value; constraints is_constructible. If the constructor
// throws, *this does not contain a value and the previous value was destroyed.
// REQUIRES: exceptions
#include <optional>
#include <initializer_list>
#include <type_traits>
#include "check.hpp"

struct IL {
  int sum = 0, extra = 0;
  constexpr IL(std::initializer_list<int> il, int e = 0) : extra(e) { for (int x : il) sum += x; }
};
struct MayThrow {
  static inline int dtor = 0;
  MayThrow(int x) { if (x < 0) throw x; }
  ~MayThrow() { ++dtor; }
};
template <class O, class... A> concept can_emplace = requires(O o, A... a) { o.emplace(a...); };
static_assert(can_emplace<std::optional<IL>, std::initializer_list<int>>);
static_assert(!can_emplace<std::optional<int>, int*>);

constexpr bool test() {
  std::optional<int> o;
  int& r = o.emplace(3);
  if (&r != &*o || *o != 3) return false;
  o.emplace();
  if (*o != 0) return false;
  std::optional<IL> il;
  IL& ir = il.emplace({1, 2, 3}, 5);
  if (ir.sum != 6 || ir.extra != 5 || &ir != &*il) return false;
  static_assert(std::is_same_v<decltype(o.emplace(1)), int&>);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::optional<MayThrow> m(std::in_place, 1);
  MayThrow::dtor = 0;
  bool threw = false;
  try { m.emplace(-1); } catch (int) { threw = true; }
  CHECK(threw);
  CHECK(!m.has_value());
  CHECK(MayThrow::dtor == 1);
  return 0;
}
