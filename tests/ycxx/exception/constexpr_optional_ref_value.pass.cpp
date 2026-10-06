// optional<T&> (P2988) during constant evaluation (P3068):
// [optional.ref.observe]/7: value() is "return has_value() ? *val : throw
// bad_optional_access();", so it throws for a disengaged optional and returns the referred
// object otherwise; the referred object is not copied (a reference to it is returned). An
// optional<T&> rebinds on assignment and emplace ([optional.ref.assign]), so value() follows the
// latest binding. [optional.bad.access]: bad_optional_access derives from exception.
// XFAIL: clang Clang 23 cannot throw during constant evaluation (P3068's core-language part)
// REQUIRES: exceptions
#include <exception>
#include <optional>
#include "check.hpp"

constexpr bool ref_value() {
  int a = 1, b = 2;
  std::optional<int&> o;
  int ok = 0;
  try {
    (void)o.value();
  } catch (const std::bad_optional_access&) {
    ++ok;
  }
  o = a;
  ok += &o.value() == &a;
  o.value() = 10; // writes through to a
  ok += a == 10;
  o.emplace(b);
  ok += &o.value() == &b && a == 10;
  o.reset();
  try {
    (void)o.value();
  } catch (const std::exception&) {
    ++ok;
  }
  const std::optional<const int&> c(b);
  ok += &c.value() == &b;
  return ok == 6;
}
static_assert(ref_value());

int main() { CHECK(ref_value()); }
