// libycxx hosted runtime: the locale-dependent conversions of the chrono formatters
// ([time.format]/2-3 with the L option): the formatting locale's time_put facet writes them.
#include <chrono>
#include <ctime>
#include <iterator>
#include <locale>
#include <sstream>

namespace {

template <class __charT>
void put_localized(std::basic_string<__charT>& out, const std::locale& __loc, const __ycxx::__detail::__chrono_c_tm& t,
                   char __spec, char __mod) {
  std::tm tm{};
  tm.tm_sec = t.__sec;
  tm.tm_min = t.min;
  tm.tm_hour = t.__hour;
  tm.tm_mday = t.__mday;
  tm.tm_mon = t.__mon;
  tm.tm_year = t.year;
  tm.tm_wday = t.__wday;
  tm.tm_yday = t.__yday;
  std::basic_ostringstream<__charT> __os;
  __os.imbue(__loc);
  const std::time_put<__charT>& __tp = std::use_facet<std::time_put<__charT>>(__loc);
  __tp.put(std::ostreambuf_iterator<__charT>(__os), __os, __os.fill(), &tm, __spec, __mod);
  out += __os.str();
}

} // namespace

void __ycxx::__detail::__chrono_put_localized(std::string& out, const std::locale& __loc, const __chrono_c_tm& t, char __spec,
                                        char __mod) {
  put_localized(out, __loc, t, __spec, __mod);
}
void __ycxx::__detail::__chrono_put_localized(std::wstring& out, const std::locale& __loc, const __chrono_c_tm& t, char __spec,
                                        char __mod) {
  put_localized(out, __loc, t, __spec, __mod);
}

bool __ycxx::__detail::__chrono_classic_time_put(const std::locale& __loc, char) {
  return &std::use_facet<std::time_put<char>>(__loc) == &std::use_facet<std::time_put<char>>(std::locale::classic());
}
bool __ycxx::__detail::__chrono_classic_time_put(const std::locale& __loc, wchar_t) {
  return &std::use_facet<std::time_put<wchar_t>>(__loc) ==
         &std::use_facet<std::time_put<wchar_t>>(std::locale::classic());
}
