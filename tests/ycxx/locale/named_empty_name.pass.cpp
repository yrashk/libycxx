// [locale.cons]/2-4: "" is always a valid name: the locale of the native environment, as the C
// library's setlocale(LC_ALL, "") takes it from LC_ALL / LC_* / LANG (C23 7.11.1.1). With
// LC_ALL naming a locale, locale("") has that locale's semantics. The result has a name
// ([locale.members]/5), and a locale made from that name compares equal ([locale.operators]/1).
// locale(other, "", cats) takes cats from the environment's locale ([locale.cons]/7).
#include <stdlib.h>
#include <locale>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

int main() {
  const char* de = require_locale("de_DE.UTF-8");

  CHECK(setenv("LC_ALL", de, 1) == 0);
  const std::locale env("");
  CHECK(env.name() != "*");
  CHECK(std::locale(env.name()) == env);
  CHECK(env == std::locale(de));
  CHECK(std::use_facet<std::numpunct<char>>(env).decimal_point() == ',');
  CHECK(std::use_facet<std::numpunct<wchar_t>>(env).decimal_point() == L',');
  CHECK(std::locale(std::string()) == env);

  const std::locale mixed(std::locale::classic(), "", std::locale::numeric);
  CHECK(mixed.name() != "*");
  CHECK(std::use_facet<std::numpunct<char>>(mixed).decimal_point() == ',');
  CHECK(&std::use_facet<std::ctype<char>>(mixed) == &std::use_facet<std::ctype<char>>(std::locale::classic()));

  CHECK(setenv("LC_ALL", "C", 1) == 0);
  const std::locale c("");
  CHECK(c == std::locale::classic());
  CHECK(c.name() == "C");
  CHECK(std::use_facet<std::numpunct<char>>(c).decimal_point() == '.');
  // the earlier locale is unaffected
  CHECK(std::use_facet<std::numpunct<char>>(env).decimal_point() == ',');
}
