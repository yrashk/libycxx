// FLAGS: -freflection
// XFAIL-COMPILER: clang  Clang 23 has no reflection (P2996)
// [meta.define.static]/2-6: reflect_constant_string(r) reflects a template parameter object of
// type const CharT[N + 1] holding r's characters and a null terminator, without a string
// literal's own terminator (/4), keeping embedded nulls. /8-12: reflect_constant_array(r)
// reflects a const T[N] (const array<T, 0> when r is empty), nesting for an array value type.
// /15: define_static_string returns a pointer to that array. /16-17: define_static_array
// returns a span over it, of static extent when ranges::size(r) is a constant expression,
// else dynamic_extent, and a null empty span for an empty range. /18: define_static_object
// returns a pointer to a static object equal to its argument (class, array and scalar).
// [meta.string.literal]/1: is_string_literal is true for a pointer into a string literal
// object (also past its first element), false otherwise, for each character type.
#include <meta>
#include <array>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace m = std::meta;

// reflect_constant_string
constexpr m::info hi = m::reflect_constant_string("hi");
static_assert(m::type_of(hi) == ^^const char[3]);
static_assert(m::extract<const char*>(hi)[0] == 'h' && m::extract<const char*>(hi)[2] == '\0');
static_assert(m::type_of(m::reflect_constant_string(std::string_view("abcd"))) == ^^const char[5]);
static_assert(m::type_of(m::reflect_constant_string(std::vector<char>{'a', '\0', 'b'})) == ^^const char[4]);
static_assert(m::type_of(m::reflect_constant_string(u8"x")) == ^^const char8_t[2]);
static_assert(m::type_of(m::reflect_constant_string(std::u32string_view())) == ^^const char32_t[1]);

// define_static_string
constexpr const char* s = std::define_static_string(std::string_view("hello"));
static_assert(std::string_view(s) == "hello" && s[5] == '\0');
constexpr const char* embedded = std::define_static_string(std::vector<char>{'a', '\0', 'b'});
static_assert(embedded[0] == 'a' && embedded[1] == '\0' && embedded[2] == 'b' && embedded[3] == '\0');
static_assert(std::is_same_v<decltype(std::define_static_string(L"w")), const wchar_t*>);
constexpr const char16_t* w16 = std::define_static_string(u"été");
static_assert(w16[0] == u'é' && w16[1] == u't' && w16[3] == 0);

// reflect_constant_array
static_assert(m::type_of(m::reflect_constant_array(std::vector<int>{1, 2, 3})) == ^^const int[3]);
static_assert(m::type_of(m::reflect_constant_array(std::vector<long>{})) == ^^const std::array<long, 0>);
constexpr int nested[2][2] = {{1, 2}, {3, 4}};
static_assert(m::type_of(m::reflect_constant_array(nested)) == ^^const int[2][2]);
static_assert(m::type_of(m::reflect_constant_array(std::array<short, 2>{5, 6})) == ^^const short[2]);

// define_static_array: extent and contents.
constexpr std::span<const int> dyn = std::define_static_array(std::vector<int>{4, 5, 6});
static_assert(dyn.size() == 3 && dyn[0] == 4 && dyn[2] == 6);
static_assert(std::is_same_v<decltype(std::define_static_array(std::vector<int>{1})), std::span<const int>>);
static_assert(std::is_same_v<decltype(std::define_static_array(std::array<int, 3>{1, 2, 3})), std::span<const int, 3>>);
constexpr auto fixed = std::define_static_array(std::array<int, 3>{7, 8, 9});
static_assert(fixed[1] == 8 && fixed.size() == 3);
constexpr std::span<const int> none = std::define_static_array(std::vector<int>{});
static_assert(none.empty() && none.data() == nullptr);
constexpr auto chars = std::define_static_array(std::string_view("xy")); // not a string: no terminator added
static_assert(chars.size() == 2 && chars[1] == 'y');

// define_static_object
struct Point {
  int x, y;
};
constexpr const Point* pp = std::define_static_object(Point{3, 4});
static_assert(pp->x == 3 && pp->y == 4);
static_assert(std::is_same_v<decltype(std::define_static_object(Point{})), const Point*>);
constexpr const int* pi = std::define_static_object(42);
static_assert(*pi == 42);
constexpr int arr3[3] = {1, 2, 3};
constexpr const int (*pa)[3] = std::define_static_object(arr3);
static_assert((*pa)[2] == 3);
static_assert(std::is_same_v<decltype(std::define_static_object(arr3)), const int (*)[3]>);

// is_string_literal
constexpr const char* lit = "literal";
static_assert(std::is_string_literal(lit) && std::is_string_literal(lit + 3));
static_assert(std::is_string_literal(L"w") && std::is_string_literal(u8"u") && std::is_string_literal(u"x") &&
              std::is_string_literal(U"y"));
constexpr char local[] = "copy";
static_assert(!std::is_string_literal(local));
static_assert(!std::is_string_literal(s)); // a template parameter object, not a string literal
static_assert(std::is_same_v<decltype(std::is_string_literal("")), bool>);

int main() {}
