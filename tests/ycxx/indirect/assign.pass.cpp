// [indirect.assign]: copy assignment copies the owned object (via T's copy assignment when
// both have one and the allocators are equal) or makes *this valueless when other is;
// self-assignment has no effect. Move assignment takes ownership (other becomes valueless)
// and is noexcept when POCMA or is_always_equal. operator=(U&&) constructs an owned object if
// *this is valueless, otherwise assigns **this = std::forward<U>(u).
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include "check.hpp"

using I = std::indirect<std::string>;
static_assert(std::is_nothrow_move_assignable_v<I>);
static_assert(std::is_copy_assignable_v<I>);
static_assert(std::is_assignable_v<I&, const char*>);
static_assert(!std::is_assignable_v<I&, int*>);

int main() {
  I a(std::string("alpha")), b(std::string("beta"));
  std::string* pa = &*a;
  a = b;
  CHECK(*a == "beta" && *b == "beta");
  CHECK(&*a == pa);  // equal allocators: **this = *other, no reallocation
  a = a;             // self-assignment
  CHECK(*a == "beta");

  I c(std::string("gamma"));
  std::string* pc = &*c;
  a = std::move(c);
  CHECK(*a == "gamma" && &*a == pc && c.valueless_after_move());

  // Assigning a valueless object makes *this valueless.
  I d(std::string("delta"));
  d = c;
  CHECK(d.valueless_after_move());
  I e(std::string("eps"));
  e = std::move(d);
  CHECK(e.valueless_after_move() && d.valueless_after_move());

  // Assigning to a valueless object.
  e = b;
  CHECK(!e.valueless_after_move() && *e == "beta");
  d = std::move(e);
  CHECK(*d == "beta");
  e = "zeta";  // operator=(U&&) on a valueless object constructs
  CHECK(!e.valueless_after_move() && *e == "zeta");
  std::string* pe = &*e;
  e = std::string("eta");  // otherwise assigns to the owned object
  CHECK(*e == "eta" && &*e == pe);
  static_assert(std::is_same_v<decltype(e = "x"), I&>);
  static_assert(std::is_same_v<decltype(e = e), I&>);
  return 0;
}
