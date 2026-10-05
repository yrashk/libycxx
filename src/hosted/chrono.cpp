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
bool to_utf8(std::string& out, const std::locale& loc, const std::string& text) {
  if constexpr (ycxx::detail::uni::encoding<char> != 8) {
    return false;
  } else {
    if (text.empty())
      return false;
    const std::text_encoding e = loc.encoding();
    using id = std::text_encoding::id;
    if (e.mib() == id::UTF8 || e.mib() == id::ASCII || e.mib() == id::unknown || e.mib() == id::other)
      return false;
    if (ycxx::detail::cfg::darwin && e.mib() != id::ISOLatin1)
      return false;
    using CV = std::codecvt<wchar_t, char, std::mbstate_t>;
    if (!std::has_facet<CV>(loc))
      return false;
    const CV& cv = std::use_facet<CV>(loc);
    std::wstring wide(text.size(), L'\0'); // a single-byte or multibyte encoding: at most one per byte
    std::mbstate_t st{};
    const char* from_next;
    wchar_t* to_next;
    if (cv.in(st, text.data(), text.data() + text.size(), from_next, wide.data(), wide.data() + wide.size(),
              to_next) != std::codecvt_base::ok || from_next != text.data() + text.size())
      return false;
    std::string r;
    for (const wchar_t* p = wide.data(); p != to_next; ++p) {
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
  // The zone of the formatted object, else none: strftime's %Z and %z would otherwise show the
  // process's time zone (tzname) for a value that has no zone or another one. tm_isdst < 0
  // writes no %z; tm_zone and tm_gmtoff are BSD members both C libraries have.
  tm.tm_isdst = t.has_offset ? 0 : -1;
  if constexpr (requires { tm.tm_zone; tm.tm_gmtoff; }) {
    tm.tm_zone = const_cast<decltype(tm.tm_zone)>(t.zone);
    tm.tm_gmtoff = static_cast<decltype(tm.tm_gmtoff)>(t.offset);
  }
  std::basic_ostringstream<charT> os;
  os.imbue(loc);
  const std::time_put<charT>& tp = std::use_facet<std::time_put<charT>>(loc);
  tp.put(std::ostreambuf_iterator<charT>(os), os, os.fill(), &tm, spec, mod);
  if constexpr (std::is_same_v<charT, char>) {
    if (to_utf8(out, loc, os.str()))
      return;
  }
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

bool ycxx::detail::chrono_classic_time_put(const std::locale& loc, char) {
  return &std::use_facet<std::time_put<char>>(loc) == &std::use_facet<std::time_put<char>>(std::locale::classic());
}
bool ycxx::detail::chrono_classic_time_put(const std::locale& loc, wchar_t) {
  return &std::use_facet<std::time_put<wchar_t>>(loc) ==
         &std::use_facet<std::time_put<wchar_t>>(std::locale::classic());
}
