// [facet.num.get.virtuals]/6-: bool without boolalpha: read as a long; 0 -> false, 1 -> true,
// any other value -> true with failbit; with boolalpha: the longest unique match of
// truename() / falsename() ("true" / "false"); otherwise false with failbit.
// COUNTERPART: libcxx:localization/locale.categories/category.numeric/locale.num.get/facet.num.get.members/get_bool.pass.cpp
#include <sstream>
#include <ios>
#include "check.hpp"

static bool get(const char* s, bool alpha, bool* failed) {
  std::istringstream is(s);
  if (alpha) is >> std::boolalpha;
  bool b = false;
  is >> b;
  *failed = is.fail();
  return b;
}

int main() {
  bool f;
  CHECK(get("1", false, &f) == true && !f);
  CHECK(get("0", false, &f) == false && !f);
  CHECK(get("2", false, &f) == true && f);
  CHECK(get("x", false, &f) == false && f);
  CHECK(get("true", true, &f) == true && !f);
  CHECK(get("false", true, &f) == false && !f);
  CHECK(get("tru", true, &f) == false && f);
  CHECK(get("yes", true, &f) == false && f);
  CHECK(get("1", true, &f) == false && f);
  std::istringstream two("false true");
  two >> std::boolalpha;
  bool a = true, b = false;
  two >> a >> b;
  CHECK(!a && b && !two.fail());
  return 0;
}
