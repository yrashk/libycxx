// [template.bitset.general]/3-12: bitset::reference "simulates a reference to a single bit".
// operator=(bool), operator=(const reference&), the const operator=(bool) const (C++26),
// operator bool, operator~, flip(), and the friend swap overloads (reference/reference,
// reference/bool&, bool&/reference), all constexpr and noexcept.
// [bitset.members]/34: "(*this)[pos] == this->test(pos)" and "(*this)[pos] = val is equivalent
// to this->set(pos, val)".
#include <bitset>
#include <type_traits>
#include <utility>
#include "check.hpp"

using B = std::bitset<16>;
using R = B::reference;

static_assert(std::is_same_v<decltype(std::declval<B&>()[0]), R>);
static_assert(std::is_same_v<decltype(std::declval<const B&>()[0]), bool>);
static_assert(std::is_nothrow_copy_constructible_v<R>);
static_assert(std::is_same_v<decltype(std::declval<R&>() = true), R&>);
static_assert(std::is_same_v<decltype(std::declval<R&>() = std::declval<const R&>()), R&>);
static_assert(std::is_same_v<decltype(std::declval<const R&>() = true), const R&>);
static_assert(noexcept(std::declval<R&>() = true));
static_assert(noexcept(std::declval<R&>() = std::declval<const R&>()));
static_assert(noexcept(std::declval<const R&>() = true));
static_assert(noexcept(static_cast<bool>(std::declval<const R&>())));
static_assert(std::is_same_v<decltype(~std::declval<const R&>()), bool>);
static_assert(noexcept(~std::declval<const R&>()));
static_assert(std::is_same_v<decltype(std::declval<R&>().flip()), R&>);
static_assert(noexcept(std::declval<R&>().flip()));
static_assert(std::is_convertible_v<R, bool>);

constexpr bool test() {
  B b;
  R r = b[3];
  R& self = (r = true);
  if (&self != &r || !b.test(3) || !r) return false;
  b[4] = b[3];  // reference = reference
  if (!b.test(4)) return false;
  r = false;
  if (b.test(3) || !~r) return false;
  if (&r.flip() != &r || !b.test(3)) return false;
  const R cr = b[5];
  cr = true;  // C++26 const assignment
  if (!b.test(5)) return false;
  R copy(r);  // refers to the same bit
  copy = false;
  if (b.test(3)) return false;
  if (b[4] != b.test(4)) return false;

  // swap overloads
  b.reset();
  b[0] = true;
  swap(b[0], b[1]);
  if (b.test(0) || !b.test(1)) return false;
  bool x = false;
  swap(b[1], x);
  if (b.test(1) || !x) return false;
  bool y = false;
  b[2] = true;
  swap(y, b[2]);
  if (!y || b.test(2)) return false;
  return true;
}
static_assert(test());

static_assert(noexcept(swap(std::declval<R>(), std::declval<R>())));
static_assert(noexcept(swap(std::declval<R>(), std::declval<bool&>())));
static_assert(noexcept(swap(std::declval<bool&>(), std::declval<R>())));

int main() {
  CHECK(test());
  return 0;
}
