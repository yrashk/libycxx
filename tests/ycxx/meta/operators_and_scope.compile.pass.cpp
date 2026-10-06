// FLAGS: -freflection
// XFAIL-COMPILER: clang  Clang 23 has no reflection (P2996)
// [meta.reflection.operators]/1 Table 63: the operators enumerators are distinct, and
// symbol_of / u8symbol_of (/4) return each one's operator symbol name; /2: operator_of(r) is the
// enumerator of the operator function (or template) r names; /3, /5: meta::exception for a
// reflection of something else and for a value that is no enumerator.
// [meta.reflection.scope]/5-16: current_function() is the enclosing function, current_class()
// the enclosing class (or the class of the enclosing member function), current_namespace() the
// enclosing namespace (or that of the enclosing entity); /9, /13: meta::exception outside a
// function / a class or member function.
// REQUIRES: exceptions
#include <meta>
#include <string_view>
#include <type_traits>

namespace m = std::meta;
using enum m::operators;

struct Row {
  m::operators op;
  std::string_view sym;
};
constexpr Row table[] = {
    {op_new, "new"}, {op_delete, "delete"}, {op_array_new, "new[]"}, {op_array_delete, "delete[]"},
    {op_co_await, "co_await"}, {op_parentheses, "()"}, {op_square_brackets, "[]"}, {op_arrow, "->"},
    {op_arrow_star, "->*"}, {op_tilde, "~"}, {op_exclamation, "!"}, {op_plus, "+"}, {op_minus, "-"},
    {op_star, "*"}, {op_slash, "/"}, {op_percent, "%"}, {op_caret, "^"}, {op_ampersand, "&"},
    {op_equals, "="}, {op_pipe, "|"}, {op_plus_equals, "+="}, {op_minus_equals, "-="},
    {op_star_equals, "*="}, {op_slash_equals, "/="}, {op_percent_equals, "%="}, {op_caret_equals, "^="},
    {op_ampersand_equals, "&="}, {op_pipe_equals, "|="}, {op_equals_equals, "=="},
    {op_exclamation_equals, "!="}, {op_less, "<"}, {op_greater, ">"}, {op_less_equals, "<="},
    {op_greater_equals, ">="}, {op_spaceship, "<=>"}, {op_ampersand_ampersand, "&&"}, {op_pipe_pipe, "||"},
    {op_less_less, "<<"}, {op_greater_greater, ">>"}, {op_less_less_equals, "<<="},
    {op_greater_greater_equals, ">>="}, {op_plus_plus, "++"}, {op_minus_minus, "--"}, {op_comma, ","},
};
static_assert(std::size(table) == 44);
static_assert(std::is_enum_v<m::operators> && std::is_scoped_enum_v<m::operators>);

consteval bool symbols_match() {
  for (const Row& r : table) {
    if (m::symbol_of(r.op) != r.sym) return false;
    std::u8string_view u = m::u8symbol_of(r.op);
    if (u.size() != r.sym.size()) return false;
    for (std::size_t i = 0; i < u.size(); ++i)
      if (static_cast<char>(u[i]) != r.sym[i]) return false;
  }
  for (const Row& a : table)
    for (const Row& b : table)
      if (&a != &b && a.op == b.op) return false;
  return true;
}
static_assert(symbols_match());
static_assert(std::is_same_v<decltype(m::symbol_of(op_plus)), std::string_view>);
static_assert(std::is_same_v<decltype(m::u8symbol_of(op_plus)), std::u8string_view>);

struct Num {
  int v;
  Num operator+(Num o) const { return {v + o.v}; }
  Num& operator+=(Num o) { v += o.v; return *this; }
  bool operator==(const Num&) const = default;
  template <class T> Num operator<<(T) const { return *this; }
  int operator()(int x) const { return x; }
  int operator[](int, int) const { return 0; }
  void plain() {}
};
Num operator-(Num a, Num b) { return {a.v - b.v}; }

static_assert(m::operator_of(^^Num::operator+) == op_plus);
static_assert(m::operator_of(^^Num::operator+=) == op_plus_equals);
static_assert(m::operator_of(^^Num::operator==) == op_equals_equals);
static_assert(m::operator_of(^^Num::operator<<) == op_less_less); // a template
static_assert(m::operator_of(^^Num::operator()) == op_parentheses);
static_assert(m::operator_of(^^Num::operator[]) == op_square_brackets);
static_assert(m::operator_of(^^::operator-) == op_minus);

template <class F>
consteval bool throws(F f) {
  try {
    f();
  } catch (const m::exception&) {
    return true;
  }
  return false;
}
static_assert(throws([] { (void)m::operator_of(^^Num::plain); }));
static_assert(throws([] { (void)m::operator_of(^^Num); }));
static_assert(throws([] { (void)m::symbol_of(static_cast<m::operators>(0x7fff)); }));

// current_function / current_class / current_namespace
namespace outer::inner {
consteval m::info here_fn() { return m::current_function(); }
static_assert(here_fn() == (^^here_fn));
static_assert(m::current_namespace() == (^^outer::inner));
struct C {
  static consteval m::info fn() { return m::current_function(); }
  static consteval m::info cls() { return m::current_class(); }
  static consteval m::info ns() { return m::current_namespace(); }
  static constexpr m::info in_class = m::current_class();
  struct Nested {
    static constexpr m::info me = m::current_class();
  };
};
static_assert(C::fn() == (^^C::fn) && C::cls() == (^^C) && C::ns() == (^^outer::inner));
static_assert(C::in_class == (^^C) && C::Nested::me == (^^C::Nested));
}
static_assert(m::current_namespace() == (^^::));
// /13: current_class() inside a function that is not a member function throws.
consteval bool current_class_throws_in_free_function() {
  try {
    (void)m::current_class();
  } catch (const m::exception&) {
    return true;
  }
  return false;
}
static_assert(current_class_throws_in_free_function());
// /4.3: inside a lambda, the current function is the closure's call operator.
constexpr auto lam = [] { return m::current_function(); };
static_assert(m::is_function(lam()) && m::parent_of(lam()) == m::remove_cv(m::type_of(^^lam)) && m::current_namespace() == (^^::));

int main() {}
