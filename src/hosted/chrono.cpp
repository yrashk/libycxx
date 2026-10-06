// libycxx hosted runtime: the locale-dependent conversions of the chrono formatters
// ([time.format]/2-3 with the L option): the formatting locale's time_put facet writes them,
// converted to UTF-8 from a named locale's own encoding (to_utf8).
#include <chrono>
#include <ctime>
#include <iterator>
#include <locale>
#include <sstream>
#include <text_encoding>
#include <ycxx/core/format_unicode.hpp>

namespace {

// [time.format]/3: with a Unicode literal encoding, the replacements of a locale among an
// implementation-defined set are converted to it. The set: the locales whose LC_CTYPE has a
// known encoding other than UTF-8 and US-ASCII, and whose wide characters are Unicode (on glibc
// all; on Darwin, whose wchar_t is not Unicode in most single-byte locales, ISO-8859-1 only).
// The text goes through the locale's codecvt<wchar_t, char, mbstate_t> to UTF-8. Appends to
// out and returns true when it converts.
bool to_utf8(std::string& out, const std::locale& __loc, const std::string& __text) {
  if constexpr (__ycxx::__detail::__uni::encoding<char> != 8) {
    return false;
  } else {
    if (__text.empty())
      return false;
    const std::text_encoding e = __loc.encoding();
    using id = std::text_encoding::id;
    if (e.mib() == id::UTF8 || e.mib() == id::ASCII || e.mib() == id::unknown || e.mib() == id::other)
      return false;
    if (__ycxx::__detail::__cfg::__darwin && e.mib() != id::ISOLatin1)
      return false;
    using CV = std::codecvt<wchar_t, char, std::mbstate_t>;
    if (!std::has_facet<CV>(__loc))
      return false;
    const CV& __cv = std::use_facet<CV>(__loc);
    std::wstring __wide(__text.size(), L'\0'); // a single-byte or multibyte encoding: at most one per byte
    std::mbstate_t __st{};
    const char* __from_next;
    wchar_t* __to_next;
    if (__cv.in(__st, __text.data(), __text.data() + __text.size(), __from_next, __wide.data(), __wide.data() + __wide.size(),
              __to_next) != std::codecvt_base::ok || __from_next != __text.data() + __text.size())
      return false;
    std::string r;
    for (const wchar_t* p = __wide.data(); p != __to_next; ++p) {
      const char32_t c = static_cast<char32_t>(*p);
      if (c < 0x80) {
        r += static_cast<char>(c);
      } else if (c < 0x800) {
        r += static_cast<char>(0xC0 | (c >> 6));
        r += static_cast<char>(0x80 | (c & 0x3F));
      } else if (c < 0x10000) {
        r += static_cast<char>(0xE0 | (c >> 12));
        r += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        r += static_cast<char>(0x80 | (c & 0x3F));
      } else if (c < 0x110000) {
        r += static_cast<char>(0xF0 | (c >> 18));
        r += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        r += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        r += static_cast<char>(0x80 | (c & 0x3F));
      } else {
        return false;
      }
    }
    out += r;
    return true;
  }
}

// tm_zone (const char* in glibc, char* in Darwin's libc) and tm_gmtoff, where the C library's tm
// has them (a dependent requires-expression: false rather than ill-formed elsewhere).
template <class TM>
void set_zone(TM& tm, const __ycxx::__detail::__chrono_c_tm& t) {
  if constexpr (requires { tm.tm_zone; tm.tm_gmtoff; }) {
    tm.tm_zone = const_cast<decltype(tm.tm_zone)>(t.__zone);
    tm.tm_gmtoff = static_cast<decltype(tm.tm_gmtoff)>(t.offset);
  }
}

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
  // The zone of the formatted object, else none: strftime's %Z and %z would otherwise show the
  // process's time zone (tzname) for a value that has no zone or another one. tm_isdst < 0
  // writes no %z; tm_zone and tm_gmtoff are BSD members both C libraries have.
  tm.tm_isdst = t.__has_offset ? 0 : -1;
  set_zone(tm, t);
  std::basic_ostringstream<__charT> __os;
  __os.imbue(__loc);
  const std::time_put<__charT>& __tp = std::use_facet<std::time_put<__charT>>(__loc);
  __tp.put(std::ostreambuf_iterator<__charT>(__os), __os, __os.fill(), &tm, __spec, __mod);
  if constexpr (std::is_same_v<__charT, char>) {
    if (to_utf8(out, __loc, __os.str()))
      return;
  }
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
