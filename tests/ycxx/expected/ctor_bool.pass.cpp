// [expected.object.cons]/23.6: for T = cv bool the U&& constructor excludes U that is an
// expected; /18.3 keeps the expected<U,G> converting constructors for bool. Hence
// expected<bool, E>(expected<int, E>(unexpect, e)) holds the *error*, not a bool.
// For other T constructible from expected<U,G>, the converting constructors are excluded and
// U&& wraps the whole source expected as a value.
#include <expected>
#include <type_traits>
#include "check.hpp"

struct FromExp {
  bool had_value;
  constexpr FromExp(const std::expected<int, int>& e) : had_value(e.has_value()) {}
};

constexpr bool test() {
  {
    std::expected<int, int> err(std::unexpect, 3);
    std::expected<bool, int> b(err);
    if (b.has_value() || b.error() != 3) return false;
    std::expected<bool, int> t(std::expected<int, int>(5));
    if (!t.has_value() || *t != true) return false;
    std::expected<bool, int> f(std::expected<int, int>(0));
    if (!f.has_value() || *f != false) return false;
  }
  {
    std::expected<int, int> err(std::unexpect, 3);
    std::expected<FromExp, int> w(err);  // U&& constructor: value built from the expected
    if (!w.has_value() || w->had_value) return false;
  }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
