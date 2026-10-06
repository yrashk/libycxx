// The non-throwing exception specifications of <locale>'s synopses:
// [locale.general] (class locale): locale() noexcept, locale(const locale&) noexcept,
//   operator=(const locale&) noexcept ([locale.cons]/1, /16);
// [locale.codecvt.general]: encoding(), always_noconv(), max_length() and their do_ virtuals
//   are noexcept (so for codecvt_byname, [locale.codecvt.byname], which inherits them);
// [facet.ctype.special.general]: ctype<char>::table() and classic_table() are noexcept.
// [res.on.exception.handling]/5: an implementation may strengthen other specifications, so
// only the required ones are checked.
#include <locale>
#include <type_traits>
#include <utility>

using wcvt = std::codecvt<wchar_t, char, std::mbstate_t>;
using u8cvt = std::codecvt<char32_t, char8_t, std::mbstate_t>;

static_assert(noexcept(std::locale()));
static_assert(std::is_nothrow_default_constructible_v<std::locale>);
static_assert(std::is_nothrow_copy_constructible_v<std::locale>);
static_assert(std::is_nothrow_copy_assignable_v<std::locale>);
static_assert(std::is_nothrow_destructible_v<std::locale>);
static_assert(std::is_same_v<decltype(std::declval<std::locale&>() = std::declval<const std::locale&>()),
                             const std::locale&>);

template <class C>
constexpr bool codecvt_noexcept() {
  return noexcept(std::declval<const C&>().encoding()) && noexcept(std::declval<const C&>().always_noconv()) &&
         noexcept(std::declval<const C&>().max_length());
}
static_assert(codecvt_noexcept<std::codecvt<char, char, std::mbstate_t>>());
static_assert(codecvt_noexcept<wcvt>());
static_assert(codecvt_noexcept<u8cvt>());
static_assert(codecvt_noexcept<std::codecvt<char16_t, char8_t, std::mbstate_t>>());
static_assert(codecvt_noexcept<std::codecvt_byname<wchar_t, char, std::mbstate_t>>());

// The do_ virtuals: an override must not be potentially-throwing ([except.spec]/8), so a
// derived facet that declares them noexcept is well-formed.
struct my_cvt : wcvt {
protected:
  int do_encoding() const noexcept override { return 1; }
  bool do_always_noconv() const noexcept override { return false; }
  int do_max_length() const noexcept override { return 1; }
};

static_assert(noexcept(std::declval<const std::ctype<char>&>().table()));
static_assert(noexcept(std::ctype<char>::classic_table()));
static_assert(std::is_same_v<decltype(std::ctype<char>::classic_table()), const std::ctype_base::mask*>);

int main() { return 0; }
