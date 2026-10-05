// [format.formatter.spec]/2-/7, [format.formattable]: the enabled specializations for
// char/wchar_t, strings, integers, bool, floating-point types and pointers; disabled
// specializations (wide strings or wchar_t with char, and types without a formatter) are not
// default constructible, copyable, or movable (/7); formattable reflects this.
// COUNTERPART: libcxx:utilities/format/format.formattable/concept.formattable.compile.pass.cpp
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>

template <class F>
constexpr bool disabled = !std::is_default_constructible_v<F> && !std::is_copy_constructible_v<F> &&
                          !std::is_move_constructible_v<F> && !std::is_copy_assignable_v<F> &&
                          !std::is_move_assignable_v<F>;

struct Unformattable {};

static_assert(disabled<std::formatter<Unformattable, char>>);
static_assert(disabled<std::formatter<wchar_t, char>>);
static_assert(disabled<std::formatter<const wchar_t*, char>>);
static_assert(disabled<std::formatter<char*, wchar_t>>);
static_assert(disabled<std::formatter<const char*, wchar_t>>);
static_assert(disabled<std::formatter<char[4], wchar_t>>);
static_assert(disabled<std::formatter<std::string, wchar_t>>);
static_assert(disabled<std::formatter<std::string_view, wchar_t>>);

static_assert(std::is_default_constructible_v<std::formatter<char, char>>);
static_assert(std::is_default_constructible_v<std::formatter<char, wchar_t>>);
static_assert(std::is_default_constructible_v<std::formatter<int, char>>);
static_assert(std::is_default_constructible_v<std::formatter<unsigned long long, wchar_t>>);
static_assert(std::is_default_constructible_v<std::formatter<bool, char>>);
static_assert(std::is_default_constructible_v<std::formatter<long double, char>>);
static_assert(std::is_default_constructible_v<std::formatter<std::nullptr_t, char>>);
static_assert(std::is_default_constructible_v<std::formatter<const void*, char>>);
static_assert(std::is_default_constructible_v<std::formatter<char[3], char>>);
static_assert(std::is_copy_constructible_v<std::formatter<std::string, char>>);

static_assert(std::formattable<int, char> && std::formattable<int, wchar_t>);
static_assert(std::formattable<double, char> && std::formattable<const char*, char>);
static_assert(std::formattable<std::string, char> && std::formattable<std::wstring, wchar_t>);
static_assert(!std::formattable<std::wstring, char> && !std::formattable<std::string, wchar_t>);
static_assert(!std::formattable<Unformattable, char>);
static_assert(std::formattable<char, wchar_t> && !std::formattable<wchar_t, char>);
static_assert(std::formattable<void*, char> && std::formattable<std::nullptr_t, char>);
static_assert(!std::formattable<int*, char>); // only void pointers are formattable

// Debug-enabled specializations provide set_debug_format() (/2).
template <class T>
concept debug_enabled = requires(std::formatter<T, char> f) { f.set_debug_format(); };
static_assert(debug_enabled<char> && debug_enabled<const char*> && debug_enabled<std::string>);
static_assert(debug_enabled<std::string_view> && debug_enabled<char[2]>);
static_assert(!debug_enabled<int>);
