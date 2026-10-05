// libycxx hosted runtime: what src/hosted/locale.cpp and src/hosted/locale_named.cpp share
// about locale names (not installed).
#pragma once

#include <locale>
#include <string>

namespace ycxx::detail {

// The six categories, in the order of the composite names: LC_COLLATE, LC_CTYPE, LC_MONETARY,
// LC_NUMERIC, LC_TIME, LC_MESSAGES (index = the bit of std::locale::category, from collate).
inline constexpr int locale_ncategories = 6;
constexpr int category_index(int cat) noexcept { return __builtin_ctz(static_cast<unsigned>(cat)) - 4; }

// A single (not composite) name with the classic semantics, normalized: "C" for "C" and
// "POSIX", the name itself for "C.UTF-8" / "C.utf8"; null for any other name.
const char* classic_locale_name(const char* name) noexcept;
// The name of category c (an index) that `name` designates: name itself, the environment's name
// for "" ([locale.cons]/4; "C" when the environment names no valid locale), or a composite
// name's part. False for a malformed composite name. (src/hosted/locale.cpp)
bool locale_name_part(const char* name, int c, std::string& out);
// Whether the C library has category c (an index) of the locale `name` (a single name).
bool named_exists(const char* name, int c);
// The codeset (nl_langinfo CODESET) of the LC_CTYPE of a single name without classic
// semantics; empty if the C library has no such locale.
std::string named_codeset(const char* name);

} // namespace ycxx::detail
