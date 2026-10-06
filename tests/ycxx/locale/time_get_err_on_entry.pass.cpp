// [locale.time.get.virtuals]: do_get_weekday, do_get_monthname and do_get_year only add to err
// ("err |="): a state already holding failbit on entry does not keep a successful read from
// storing its field.
#include <ctime>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"

template <class charT>
static void run(const charT* month, const charT* day, const charT* year) {
  using It = typename std::basic_string<charT>::const_iterator;
  std::basic_istringstream<charT> is;
  struct facet : std::time_get<charT, It> {
    facet() : std::time_get<charT, It>(1) {}
  } tg;
  const std::ios_base::iostate entry = std::ios_base::failbit | std::ios_base::eofbit;
  std::tm t{};
  std::ios_base::iostate err = entry;
  const std::basic_string<charT> m(month), d(day), y(year);
  tg.get_monthname(m.begin(), m.end(), is, err, &t);
  CHECK(t.tm_mon == 8);
  CHECK((err & entry) == entry);
  err = entry;
  tg.get_weekday(d.begin(), d.end(), is, err, &t);
  CHECK(t.tm_wday == 3);
  err = entry;
  tg.get_year(y.begin(), y.end(), is, err, &t);
  CHECK(t.tm_year == 2024 - 1900);
}

int main() {
  run<char>("September ", "Wednesday ", "2024 ");
  run<wchar_t>(L"September ", L"Wednesday ", L"2024 ");
}
