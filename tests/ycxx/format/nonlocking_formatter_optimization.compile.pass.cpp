// [format.syn], [format.formatter.locking]: `template<class T> constexpr bool
// enable_nonlocking_formatter_optimization = false;`. [format.formatter.spec]/3: "Unless
// otherwise specified, for each type T for which a formatter specialization is provided by
// the library, each of the headers provides ... enable_nonlocking_formatter_optimization<T>
// = true" -- checked for the specializations of /2.1-/2.6 (characters, string types, integer
// types and bool, floating-point types, nullptr_t, void*, const void*). Otherwise specified:
// [format.syn] ranges with format_kind<R> != disabled: false; [format.tuple]/1: pair / tuple:
// the conjunction over the element types; [time.format]/8-/9: chrono::duration<Rep, Period>:
// that of Rep, and zoned_time only for TimeZonePtr = const time_zone*. A program-defined
// type keeps the primary template's false.
// COUNTERPART: libcxx:utilities/format/format.formatter/format.formatter.locking/enable_nonlocking_formatter_optimization.compile.pass.cpp
#include <format>
#include <chrono>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

template <class T>
constexpr bool nl = std::enable_nonlocking_formatter_optimization<T>;

struct User {};
template <>
struct std::formatter<User> : std::formatter<int> {
  auto format(User, std::format_context& ctx) const { return std::formatter<int>::format(1, ctx); }
};

static_assert(std::is_same_v<decltype(std::enable_nonlocking_formatter_optimization<User>), const bool>);
// /2.1, /2.3, /2.4
static_assert(nl<char> && nl<wchar_t> && nl<bool>);
static_assert(nl<signed char> && nl<unsigned char> && nl<short> && nl<unsigned short> && nl<int> && nl<unsigned>);
static_assert(nl<long> && nl<unsigned long> && nl<long long> && nl<unsigned long long>);
static_assert(nl<float> && nl<double> && nl<long double>);
// /2.2
static_assert(nl<char*> && nl<const char*> && nl<wchar_t*> && nl<const wchar_t*>);
static_assert(nl<char[4]> && nl<wchar_t[1]>);
static_assert(nl<std::string> && nl<std::wstring> && nl<std::string_view> && nl<std::wstring_view>);
static_assert(nl<std::basic_string<char, std::char_traits<char>, std::allocator<char>>>);
// /2.5, /2.6
static_assert(nl<std::nullptr_t> && nl<void*> && nl<const void*>);
// program-defined
static_assert(!nl<User>);
// ranges
static_assert(!nl<std::vector<int>> && !nl<std::map<int, int>> && !nl<std::vector<std::string>>);
// pair / tuple
static_assert(nl<std::pair<int, std::string>> && nl<std::tuple<>> && nl<std::tuple<char, double, const char*>>);
static_assert(!nl<std::pair<int, User>> && !nl<std::tuple<int, std::vector<int>>>);
static_assert(nl<std::pair<std::pair<int, int>, std::tuple<bool>>> && !nl<std::tuple<std::pair<User, int>>>);
// chrono
static_assert(nl<std::chrono::duration<int, std::milli>> && nl<std::chrono::duration<double>>);
static_assert(nl<std::chrono::zoned_time<std::chrono::seconds>>);
static_assert(!nl<std::chrono::zoned_time<std::chrono::seconds, std::chrono::time_zone*>>);
