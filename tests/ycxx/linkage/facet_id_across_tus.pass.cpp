// Standard facet templates specialised with non-default iterator types, installed in one
// translation unit and looked up in another.
//   [locale.facet]/5-6: "A facet has a static member id of type locale::id"; "The id member is
//     used ... to identify a facet interface"; the static data member of a class template
//     specialization is one entity in the program ([basic.def.odr]/15, [temp.spec.general]),
//     so &F::id is the same address in every translation unit.
//   [locale.global.templates]/1-3: has_facet<F>(loc) is true and use_facet<F>(loc) returns the
//     installed facet when loc contains F, whichever translation unit installed it.
//   [locale.nm.put]: num_put<char, char*>::put writes through the char* iterator;
//   [locale.num.get]: num_get<char, const char*>::get parses from it;
//   [locale.money.put]: money_put<wchar_t, wchar_t*>::put; [locale.time.get]: time_get with
//     const char* iterators.
// FILES: ../support/linkage/facet_id_tu2.cpp
#include <ctime>
#include <ios>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"

std::locale tu2_locale();
const void* tu2_id_addresses(int which);
bool tu2_has(const std::locale& l);

int main() {
  CHECK(tu2_id_addresses(0) == &std::num_put<char, char*>::id);
  CHECK(tu2_id_addresses(1) == &std::ctype<char>::id);
  CHECK(tu2_id_addresses(2) == &std::numpunct<wchar_t>::id);
  CHECK(tu2_id_addresses(3) == &std::codecvt<char32_t, char8_t, std::mbstate_t>::id);

  std::locale l = tu2_locale();
  CHECK((std::has_facet<std::num_put<char, char*>>(l)));
  CHECK((std::has_facet<std::num_get<char, const char*>>(l)));
  CHECK((std::has_facet<std::money_put<wchar_t, wchar_t*>>(l)));
  CHECK((std::has_facet<std::time_get<char, const char*>>(l)));
  CHECK((!std::has_facet<std::num_put<wchar_t, wchar_t*>>(l)));
  CHECK(!tu2_has(std::locale::classic()) && tu2_has(l));

  std::ostringstream fmt;  // supplies flags and the locale for numpunct
  char buf[32] = {};
  char* end = std::use_facet<std::num_put<char, char*>>(l).put(buf, fmt, ' ', 12345L);
  CHECK(std::string(buf, end) == "12345");

  const char in[] = "678 rest";
  std::istringstream fmt_in;
  std::ios_base::iostate err = std::ios_base::goodbit;
  long v = 0;
  const char* stop = std::use_facet<std::num_get<char, const char*>>(l).get(in, in + sizeof in - 1, fmt_in, err, v);
  CHECK(v == 678 && stop == in + 3 && err == std::ios_base::goodbit);

  std::wostringstream wfmt;
  wfmt.imbue(std::locale::classic());
  wchar_t wbuf[32] = {};
  wchar_t* wend = std::use_facet<std::money_put<wchar_t, wchar_t*>>(l).put(wbuf, false, wfmt, L' ', 1234.0L);
  CHECK(std::wstring(wbuf, wend) == L"1234");

  const char date[] = "13:45:30";
  std::tm t{};
  err = std::ios_base::goodbit;
  const char* dend = std::use_facet<std::time_get<char, const char*>>(l).get_time(date, date + 8, fmt_in, err, &t);
  CHECK(dend == date + 8 && t.tm_hour == 13 && t.tm_min == 45 && t.tm_sec == 30);
  CHECK((err & std::ios_base::failbit) == 0);
  return 0;
}
