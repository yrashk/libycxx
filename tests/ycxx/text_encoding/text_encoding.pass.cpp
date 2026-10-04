// [text.encoding.class]: text_encoding is trivially copyable; default-constructed it is
// unknown with an empty name; text_encoding(string_view) finds a known registered encoding by
// primary name or alias using comp-name (ignoring case, non-alphanumerics and leading zeros
// not preceded by a numeric prefix; Example 1: "UTF-8" ~ "utf8" ~ "u.t.f-008", but not
// "ut8" or "utf-80"), else mib() is other, and name() is the given string; text_encoding(id)
// has that mib and a name among its aliases (empty for other/unknown); aliases() is a view
// whose front() is the primary name and which contains "ASCII" for US-ASCII; == compares
// mibs, or names with comp-name when both are other. hash<text_encoding> is enabled.
#include <text_encoding>
#include <algorithm>
#include <cstring>
#include <functional>
#include <iterator>
#include <ranges>
#include <string_view>
#include <type_traits>
#include "check.hpp"

using TE = std::text_encoding;
using std::string_view;
static_assert(std::is_trivially_copyable_v<TE>);
static_assert(TE::max_name_length == 63);
static_assert(std::is_same_v<std::underlying_type_t<TE::id>, std::int_least32_t>);
static_assert(static_cast<int>(TE::id::other) == 1 && static_cast<int>(TE::id::unknown) == 2);
static_assert(static_cast<int>(TE::id::ASCII) == 3 && static_cast<int>(TE::id::UTF8) == 106);
static_assert(static_cast<int>(TE::UTF16) == 1015);  // using enum id
static_assert(!std::is_convertible_v<string_view, TE> && std::is_convertible_v<TE::id, TE>);
static_assert(noexcept(TE(string_view("x"))) && noexcept(TE(TE::id::UTF8)) && noexcept(TE().mib()));
static_assert(std::ranges::view<TE::aliases_view> && std::ranges::random_access_range<TE::aliases_view>);
static_assert(std::ranges::borrowed_range<TE::aliases_view> && std::copyable<TE::aliases_view>);
static_assert(std::is_same_v<std::ranges::range_value_t<TE::aliases_view>, const char*>);
static_assert(std::is_same_v<std::ranges::range_reference_t<TE::aliases_view>, const char*>);

constexpr bool eqname(const char* a, const char* b) { return string_view(a) == string_view(b); }

// constexpr use.
static_assert(TE().mib() == TE::id::unknown && eqname(TE().name(), ""));
static_assert(TE(string_view("UTF-8")).mib() == TE::id::UTF8);
static_assert(TE(string_view("utf8")).mib() == TE::id::UTF8);
static_assert(TE(string_view("u.t.f-008")).mib() == TE::id::UTF8);
static_assert(TE(string_view("ut8")).mib() == TE::id::other);
static_assert(TE(string_view("utf-80")).mib() == TE::id::other);
static_assert(TE(TE::id::UTF8) == TE::id::UTF8);

int main() {
  TE def;
  CHECK(def.mib() == TE::id::unknown && *def.name() == '\0' && std::ranges::empty(def.aliases()));
  TE u8(string_view("Utf-8"));
  CHECK(u8.mib() == TE::id::UTF8 && std::strcmp(u8.name(), "Utf-8") == 0);
  CHECK(u8 == TE(TE::id::UTF8) && u8 == TE::id::UTF8 && u8 != TE::id::UTF16);
  TE ascii(string_view("ascii"));
  CHECK(ascii.mib() == TE::id::ASCII);
  TE us(string_view("US-ASCII"));
  CHECK(us.mib() == TE::id::ASCII && us == ascii);
  TE latin(string_view("ISO-8859-1"));
  CHECK(latin.mib() == TE::id::ISOLatin1);
  TE latin2(string_view("iso_8859-1"));
  CHECK(latin2 == latin);

  // Unregistered names: other, compared by name with comp-name.
  TE x(string_view("x-my-encoding"));
  CHECK(x.mib() == TE::id::other && std::strcmp(x.name(), "x-my-encoding") == 0);
  CHECK(x == TE(string_view("X_MY.ENCODING")) && x != TE(string_view("x-other")));
  CHECK(x != TE::id::unknown && x == TE::id::other);
  CHECK(std::ranges::empty(x.aliases()));

  // From id: name is one of the aliases; aliases().front() is the primary name.
  TE fromid(TE::id::UTF8);
  CHECK(*fromid.name() != '\0');
  auto al = fromid.aliases();
  CHECK(!std::ranges::empty(al) && std::strcmp(al.front(), "UTF-8") == 0);
  CHECK(std::ranges::any_of(al, [&](const char* a) { return std::strcmp(a, fromid.name()) == 0; }));
  CHECK(TE(string_view(fromid.name())).mib() == TE::id::UTF8);  // [text.encoding.general]/6
  auto asc = TE(TE::id::ASCII).aliases();
  CHECK(std::strcmp(asc.front(), "US-ASCII") == 0);
  CHECK(std::ranges::any_of(asc, [](const char* a) { return std::strcmp(a, "ASCII") == 0; }));
  CHECK(*TE(TE::id::other).name() == '\0' && *TE(TE::id::unknown).name() == '\0');

  // literal() and environment().
  constexpr TE lit = TE::literal();
  (void)lit;  // the ordinary literal encoding (implementation-defined)
  TE env = TE::environment();
  CHECK(TE::environment_is<TE::id::UTF8>() == (env == TE::id::UTF8));

  std::hash<TE> h;
  CHECK(h(u8) == h(u8));
  (void)h(def);
  return 0;
}
