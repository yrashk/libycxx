// [locale.statics]/2: locale::global(loc) "if loc has a name, ... the global C locale is
// set to it" (as setlocale(LC_ALL, loc.name().c_str())); [locale.cons]/1: locale() is then a
// copy of loc. [locale.members]/6-7: encoding() is the encoding of the locale's LC_CTYPE (the C
// library's CODESET of that name). [locale.messages.byname]: a catalog that does not exist cannot
// be opened ([locale.messages.virtuals]/2: a value less than 0), and get() then returns dfault.
#include <langinfo.h>
#include <locale.h>
#include <string.h>
#include <locale>
#include <string>
#include <text_encoding>
#include "check.hpp"
#include "named_locale.hpp"

int main() {
  const char* de = require_locale("de_DE.UTF-8");
  const char* latin1 = require_locale("de_DE.ISO8859-1");

  const std::locale prev = std::locale::global(std::locale(de));
  CHECK(prev == std::locale::classic());
  CHECK(std::locale().name() == de);
  CHECK(strcmp(setlocale(LC_ALL, nullptr), de) == 0);
  CHECK(strcmp(localeconv()->decimal_point, ",") == 0);

  // a composite name sets each category
  std::locale::global(std::locale(std::locale::classic(), de, std::locale::numeric));
  CHECK(strcmp(setlocale(LC_NUMERIC, nullptr), de) == 0);
  CHECK(strcmp(setlocale(LC_CTYPE, nullptr), "C") == 0);
  // an unnamed locale leaves the C locale alone
  std::locale::global(std::locale(std::locale(latin1), new std::numpunct<char>));
  CHECK(strcmp(setlocale(LC_NUMERIC, nullptr), de) == 0);
  std::locale::global(std::locale::classic());
  CHECK(strcmp(setlocale(LC_ALL, nullptr), "C") == 0);

  // encoding()
  const std::string cs = in_c_locale(latin1, [] { return std::string(nl_langinfo(CODESET)); });
  CHECK(std::locale(latin1).encoding() == std::text_encoding(cs));
  CHECK(std::locale(latin1).encoding().mib() == std::text_encoding::id::ISOLatin1);
  CHECK(std::locale(de).encoding().mib() == std::text_encoding::id::UTF8);

  // messages: no such catalog
  const auto& m = std::use_facet<std::messages<char>>(std::locale(de));
  const auto c = m.open("ycxx-no-such-catalog-anywhere", std::locale(de));
  CHECK(c < 0);
  const auto& wm = std::use_facet<std::messages<wchar_t>>(std::locale(de));
  CHECK(wm.open("ycxx-no-such-catalog-anywhere", std::locale(de)) < 0);
}
