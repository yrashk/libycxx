// libycxx hosted runtime: the locale-dependent conversions of the chrono formatters
// ([time.format]/2-3 with the L option): the formatting locale's time_put facet writes them.
#include <chrono>
#include <ctime>
#include <iterator>
#include <locale>
#include <sstream>

namespace {

template <class charT>
void put_localized(std::basic_string<charT>& out, const std::locale& loc, const ycxx::detail::chrono_c_tm& t,
                   char spec, char mod) {
  std::tm tm{};
  tm.tm_sec = t.sec;
  tm.tm_min = t.min;
  tm.tm_hour = t.hour;
  tm.tm_mday = t.mday;
  tm.tm_mon = t.mon;
  tm.tm_year = t.year;
  tm.tm_wday = t.wday;
  tm.tm_yday = t.yday;
  std::basic_ostringstream<charT> os;
  os.imbue(loc);
  const std::time_put<charT>& tp = std::use_facet<std::time_put<charT>>(loc);
  tp.put(std::ostreambuf_iterator<charT>(os), os, os.fill(), &tm, spec, mod);
  out += os.str();
}

} // namespace

void ycxx::detail::chrono_put_localized(std::string& out, const std::locale& loc, const chrono_c_tm& t, char spec,
                                        char mod) {
  put_localized(out, loc, t, spec, mod);
}
void ycxx::detail::chrono_put_localized(std::wstring& out, const std::locale& loc, const chrono_c_tm& t, char spec,
                                        char mod) {
  put_localized(out, loc, t, spec, mod);
}
