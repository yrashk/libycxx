// [locale.cons]: "explicit locale(const char* std_name); ... Throws: runtime_error if the argument
// is not valid, or is null." and likewise "locale(const locale& other, const char* std_name,
// category cats); ... Throws: runtime_error if the second argument is not valid, or is null."
// A null pointer constant (nullptr, 0) selects the const char* overloads: the class has no other
// constructor a null pointer converts to without a user-defined conversion.
#include <locale>
#include <stdexcept>
#include "check.hpp"

template <class F>
static bool throws_runtime_error(F f) {
  try {
    f();
  } catch (const std::runtime_error&) {
    return true;
  }
  return false;
}

int main() {
  CHECK(throws_runtime_error([] { std::locale l(nullptr); }));
  CHECK(throws_runtime_error([] { std::locale l(0); }));
  CHECK(throws_runtime_error([] { std::locale l(std::locale::classic(), nullptr, std::locale::all); }));
  CHECK(throws_runtime_error([] { std::locale l(std::locale::classic(), 0, std::locale::ctype); }));
  return 0;
}
