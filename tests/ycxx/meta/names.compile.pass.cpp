// FLAGS: -freflection
// XFAIL-COMPILER: clang  Clang 23 has no reflection (P2996)
// [meta.reflection.names]/1: has_identifier for a typedef name for linkage purposes (1.1), a
// class template specialization (1.4.1: false), an enumeration, a function, an operator
// function (1.5: false), a constructor (false), a template (1.6), a variable, an enumerator,
// a namespace and a data member description (1.13); /2-4: identifier_of and u8identifier_of
// return that identifier (the ud-suffix for a literal operator, 3.2; the name N of a data
// member description, 3.6; the base class's for a base relationship, 3.5), and throw
// meta::exception without one; /5: display_string_of and u8display_string_of return string_view
// and u8string_view; /7: source_location_of returns source_location{} for a value, a type that
// is not a class or enumeration, the global namespace and a data member description.
// REQUIRES: exceptions
#include <meta>
#include <source_location>
#include <string_view>
#include <type_traits>
#include <vector>

namespace m = std::meta;
using namespace std::string_view_literals;

typedef struct {
  int a;
} Anon; // typedef name for linkage purposes
enum Color { red };
struct Base {};
struct Widget : Base {
  int field;
  Widget();
  void method();
  bool operator==(const Widget&) const;
};
template <class T>
struct Box {};
int variable;
void function(int);
namespace space {}
constexpr unsigned long long operator""_kb(unsigned long long v) { return v * 1024; }

static_assert(m::has_identifier(^^Anon) && m::identifier_of(^^Anon) == "Anon");
static_assert(m::has_identifier(^^Color) && m::identifier_of(^^Color) == "Color");
static_assert(m::has_identifier(^^red) && m::identifier_of(^^red) == "red");
static_assert(m::has_identifier(^^Widget::field) && m::identifier_of(^^Widget::field) == "field");
static_assert(m::has_identifier(^^Widget::method) && m::u8identifier_of(^^Widget::method) == u8"method");
static_assert(!m::has_identifier(^^Widget::operator==));
static_assert(!m::has_identifier(^^Box<int>) && m::has_identifier(^^Box) && m::identifier_of(^^Box) == "Box");
static_assert(m::has_identifier(^^variable) && m::identifier_of(^^variable) == "variable");
static_assert(m::has_identifier(^^function) && m::identifier_of(^^function) == "function");
static_assert(m::has_identifier(^^space) && m::identifier_of(^^space) == "space");
static_assert(m::identifier_of(^^operator""_kb) == "_kb");
static_assert(!m::has_identifier(^^int) && !m::has_identifier(^^::) && !m::has_identifier(^^const Widget));
static_assert(m::identifier_of(m::bases_of(^^Widget, m::access_context::unchecked())[0]) == "Base");
static_assert(m::has_identifier(m::data_member_spec(^^int, {.name = "n"})) &&
              m::identifier_of(m::data_member_spec(^^int, {.name = "n"})) == "n");
static_assert(!m::has_identifier(m::data_member_spec(^^int, {.bit_width = 0})));

consteval bool ctor_has_no_identifier() {
  for (m::info mem : m::members_of(^^Widget, m::access_context::unchecked()))
    if (m::is_constructor(mem) && m::has_identifier(mem)) return false;
  return true;
}
static_assert(ctor_has_no_identifier());

static_assert(std::is_same_v<decltype(m::identifier_of(^^red)), std::string_view>);
static_assert(std::is_same_v<decltype(m::u8identifier_of(^^red)), std::u8string_view>);
static_assert(std::is_same_v<decltype(m::display_string_of(^^red)), std::string_view>);
static_assert(std::is_same_v<decltype(m::u8display_string_of(^^red)), std::u8string_view>);
static_assert(std::is_same_v<decltype(m::source_location_of(^^red)), std::source_location>);

// /7
constexpr bool empty_location(std::source_location l) {
  return l.line() == 0 && l.column() == 0 && std::string_view(l.file_name()).empty() &&
         std::string_view(l.function_name()).empty();
}
static_assert(empty_location(m::source_location_of(m::reflect_constant(3))));
static_assert(empty_location(m::source_location_of(^^int)));
static_assert(empty_location(m::source_location_of(^^int*)));
static_assert(empty_location(m::source_location_of(^^::)));
static_assert(empty_location(m::source_location_of(m::data_member_spec(^^int, {.name = "n"}))));

template <class F>
consteval bool throws(F f) {
  try {
    f();
  } catch (const m::exception&) {
    return true;
  }
  return false;
}
static_assert(throws([] { (void)m::identifier_of(^^Box<int>); }));
static_assert(throws([] { (void)m::u8identifier_of(^^::); }));
static_assert(!throws([] { (void)m::display_string_of(^^Box<int>); }));

int main() {}
