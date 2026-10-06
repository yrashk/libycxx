// libycxx hosted runtime: std::text_encoding::environment ([text.encoding.members]/13-16) and
// std::locale::encoding ([locale.members]/6-7).
#include <locale>
#include <text_encoding>
#include <ycxx/pal.h>

#include <cstring>
#include "locale_named.hpp"

namespace {

std::text_encoding from_name(const char* name) {
  const std::size_t n = std::strlen(name);
  if (n == 0 || n > std::text_encoding::max_name_length)
    return std::text_encoding();
  for (std::size_t i = 0; i < n; ++i) // [text.encoding.members]/1: basic characters only
    if (static_cast<unsigned char>(name[i]) < 0x20 || static_cast<unsigned char>(name[i]) > 0x7e)
      return std::text_encoding();
  return std::text_encoding(std::string_view(name, n));
}

} // namespace

// Determined once, on first use: the recommended practice of /16 is that later changes to the
// environment (setenv) do not affect the result; setlocale never does.
std::text_encoding std::text_encoding::environment() {
  static const text_encoding env = [] {
    char __buf[max_name_length + 2] = {};
    if (ycxx_pal_environment_encoding(__buf, sizeof __buf) != 0)
      return text_encoding();
    return from_name(__buf);
  }();
  return env;
}

// The encoding of the locale's LC_CTYPE category: US-ASCII for "C" (the POSIX locale's
// portable character set), UTF-8 for "C.UTF-8", the C library's CODESET for other names;
// unknown for a locale without a name.
std::text_encoding std::locale::encoding() const {
  const string n = name();
  if (n == "*")
    return text_encoding();
  string ctype = n;
  if (const auto __pos = n.find("LC_CTYPE="); __pos != string::npos) {
    const auto end = n.find(';', __pos);
    ctype = n.substr(__pos + 9, end == string::npos ? string::npos : end - __pos - 9);
  }
  if (ctype == "C" || ctype == "POSIX")
    return text_encoding(text_encoding::id::ASCII);
  // any other name: the C library's codeset for it (a name with the classic semantics,
  // "C.UTF-8", is named by its suffix)
  if (__ycxx::__detail::__classic_locale_name(ctype.c_str()) == nullptr) {
    const string __cs = __ycxx::__detail::__named_codeset(ctype.c_str());
    if (!__cs.empty())
      return from_name(__cs.c_str());
  }
  if (const auto dot = ctype.find('.'); dot != string::npos)
    return from_name(ctype.c_str() + dot + 1);
  return text_encoding();
}
