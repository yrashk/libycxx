// [optional.ctor]/23.4: the U&& constructor is excluded for T = cv bool when U is an optional,
// and /28.2, /33.2 make the optional<U> converting constructors used for bool even though
// bool is constructible from optional<U>. So optional<bool>(optional<int>{}) is *empty*
// (it is not "true because the source optional is engaged-ish" nor "false").
// For non-bool T constructible from optional<U> (converts-from-any-cvref true), the
// optional<U> converting constructors are excluded and U&& wraps the whole optional.
#include <optional>
#include <type_traits>
#include "check.hpp"

struct FromOpt {
  bool engaged;
  constexpr FromOpt(const std::optional<int>& o) : engaged(o.has_value()) {}
};

constexpr bool test() {
  {
    std::optional<int> empty;
    std::optional<bool> b(empty);
    if (b.has_value()) return false;
    std::optional<bool> b2(std::optional<int>(0));
    if (!b2.has_value() || *b2 != false) return false;
    std::optional<bool> b3(std::optional<int>(5));
    if (!b3.has_value() || *b3 != true) return false;
    const std::optional<int> ce;
    std::optional<const bool> b4 = ce;  // implicit: int -> bool convertible
    if (b4.has_value()) return false;
  }
  {
    std::optional<int> empty;
    std::optional<FromOpt> f(empty);  // U&& ctor: contains a FromOpt built from the optional
    if (!f.has_value() || f->engaged) return false;
    std::optional<FromOpt> g(std::optional<int>(1));
    if (!g.has_value() || !g->engaged) return false;
  }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
