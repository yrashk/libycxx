// Explicitness of basic_string's converting constructors ([basic.string.general]):
// "constexpr explicit basic_string(const Allocator& a) noexcept;" and
// "template<class T> constexpr explicit basic_string(const T& t, const Allocator& a =
// Allocator());" (string-view-like) are explicit; "constexpr basic_string(const charT* s,
// const Allocator& a = Allocator());" and the initializer_list constructor are not. So a
// const charT* and a braced list convert implicitly to a string, a string_view and an
// allocator do not (a string_view argument needs an explicit conversion), and nullptr does
// not ("basic_string(nullptr_t) = delete;"). [string.cons]/12: the string-view-like
// constructor still works with direct-initialization.
#include <string>
#include <string_view>
#include <initializer_list>
#include <memory>
#include <type_traits>

using S = std::string;
static_assert(std::is_convertible_v<const char*, S>);
static_assert(std::is_convertible_v<const char (&)[4], S>);
static_assert(std::is_convertible_v<std::initializer_list<char>, S>);
static_assert(!std::is_convertible_v<std::string_view, S>);
static_assert(std::is_constructible_v<S, std::string_view>);
static_assert(!std::is_convertible_v<std::allocator<char>, S>);
static_assert(std::is_constructible_v<S, std::allocator<char>>);
static_assert(!std::is_convertible_v<std::nullptr_t, S>);
static_assert(!std::is_constructible_v<S, std::nullptr_t>);
static_assert(!std::is_convertible_v<char, S>);
static_assert(!std::is_convertible_v<int, S>);
static_assert(!std::is_convertible_v<std::wstring_view, S>);
static_assert(!std::is_constructible_v<S, std::wstring_view>);
static_assert(std::is_convertible_v<const wchar_t*, std::wstring>);
static_assert(!std::is_convertible_v<std::wstring_view, std::wstring>);

struct ToView {
  operator std::string_view() const { return "v"; }
};
static_assert(!std::is_convertible_v<ToView, S>);
static_assert(std::is_constructible_v<S, ToView>);

void take(const S&);
template <class A>
concept passes = requires(A a) { take(a); };
static_assert(passes<const char*>);
static_assert(!passes<std::string_view>);
static_assert(!passes<ToView>);
