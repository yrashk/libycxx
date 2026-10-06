// [locale.facet]/2: "A const-qualified facet is a valid template argument to any locale function
// that expects a Facet template parameter": use_facet<const F>, has_facet<const F>, combine and
// locale(other, const F*) name the same facet as F.
// [locale.global.templates]/3: use_facet throws bad_cast if has_facet<Facet>(loc) is false; /5:
// has_facet is noexcept; /4: the reference use_facet returns remains valid at least as long as
// any copy of loc exists. [locale.facet]/3: a refs == 0 facet is deleted when the last locale
// containing it is destroyed.
// REQUIRES: exceptions
#include <locale>
#include <type_traits>
#include <typeinfo>
#include "check.hpp"

static int destroyed = 0;

struct mine : std::locale::facet {
  static std::locale::id id;
  int v;
  explicit mine(int x) : v(x) {}
  ~mine() override { ++destroyed; }
};
std::locale::id mine::id;

int main() {
  const std::locale& c = std::locale::classic();
  static_assert(noexcept(std::has_facet<mine>(c)));
  static_assert(noexcept(std::has_facet<const mine>(c)));
  static_assert(std::is_same_v<decltype(std::use_facet<const std::ctype<char>>(c)), const std::ctype<char>&>);

  // const-qualified Facet: the same facet
  CHECK(std::has_facet<const std::ctype<char>>(c));
  CHECK(&std::use_facet<const std::ctype<char>>(c) == &std::use_facet<std::ctype<char>>(c));
  CHECK(!std::has_facet<const mine>(c));
  try {
    (void)std::use_facet<const mine>(c);
    CHECK(false);
  } catch (const std::bad_cast&) {
  }
  {
    const mine* f = new mine(5);
    std::locale l(c, f); // Facet deduced as const mine
    CHECK(std::has_facet<mine>(l) && std::has_facet<const mine>(l));
    CHECK(&std::use_facet<mine>(l) == f);
    std::locale k = c.combine<const mine>(l);
    CHECK(&std::use_facet<mine>(k) == f);
  }
  CHECK(destroyed == 1);

  // the reference stays valid while any copy of the locale exists
  destroyed = 0;
  {
    std::locale* original = new std::locale(c, new mine(7));
    std::locale copy = *original;
    const mine& ref = std::use_facet<mine>(*original);
    delete original; // a copy still exists
    CHECK(destroyed == 0);
    CHECK(ref.v == 7);
    CHECK(&std::use_facet<mine>(copy) == &ref);
    copy = c; // the last copy
    CHECK(destroyed == 1);
  }
}
