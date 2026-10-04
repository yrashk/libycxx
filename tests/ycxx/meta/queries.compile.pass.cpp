// FLAGS: -freflection
// XFAIL-COMPILER: clang  Clang 23 has no reflection (P2996)
// [meta.syn]: std::meta::info is decltype(^^::).
// [meta.reflection.names]: has_identifier / identifier_of give the declared name.
// [meta.reflection.queries]: type_of, is_public/is_private, is_static_member,
//   is_nonstatic_data_member, is_type, is_function, is_variable, is_namespace, parent_of,
//   dealias, is_enumerator, has_default_member_initializer, is_bit_field.
// [meta.reflection.member.queries]: nonstatic_data_members_of, static_data_members_of,
//   bases_of and enumerators_of return the members in declaration order; access_context
//   ::unchecked() includes inaccessible members, ::unprivileged() only public ones.
// [meta.reflection.layout]: offset_of returns member_offset {bytes, bits} (total_bits() is
//   bytes * CHAR_BIT + bits); size_of, alignment_of, bit_size_of.
// [meta.reflection.extract]: extract<T>; [meta.reflection.result]: reflect_constant;
// [meta.reflection.substitute]: can_substitute, substitute.
// [meta.reflection.exception]: an invalid query throws std::meta::exception (catchable during
//   constant evaluation).
#include <meta>
#include <array>
#include <climits>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <vector>

namespace m = std::meta;
static_assert(std::is_same_v<m::info, decltype(^^::)>);

struct Base {
  int b;
};
struct S : Base {
  int a = 1;
  char c;
  static inline double sd = 2.5;
  unsigned bf : 3;
  void f();

 private:
  long hidden;
};
enum class Color { red = 2, green, blue = 10 };
namespace ns {
int var = 3;
}
using IntAlias = int;
template <class T, int N>
struct Arr {
  T data[N];
};

constexpr auto unchecked = m::access_context::unchecked();

static_assert(m::has_identifier(^^S) && m::identifier_of(^^S) == "S");
static_assert(m::identifier_of(^^S::a) == "a" && m::identifier_of(^^ns) == "ns");
static_assert(m::identifier_of(^^Color::green) == "green");
static_assert(!m::has_identifier(^^int));

static_assert(m::type_of(^^S::a) == (^^int) && m::type_of(^^S::c) == (^^char));
static_assert(m::type_of(^^ns::var) == (^^int));
// (naming S::hidden in ^^S::hidden is access-checked; find it through members_of instead)
consteval m::info hidden_member() { return m::nonstatic_data_members_of(^^S, m::access_context::unchecked())[3]; }
static_assert(m::is_public(^^S::a) && m::is_private(hidden_member()) && m::identifier_of(hidden_member()) == "hidden");
static_assert(m::is_static_member(^^S::sd) && !m::is_static_member(^^S::a));
static_assert(m::is_nonstatic_data_member(^^S::a) && !m::is_nonstatic_data_member(^^S::sd));
static_assert(m::is_type(^^S) && m::is_function(^^S::f) && m::is_variable(^^ns::var) && m::is_namespace(^^ns));
static_assert(m::parent_of(^^S::a) == (^^S) && m::parent_of(^^ns::var) == (^^ns));
static_assert(m::is_type_alias(^^IntAlias) && m::dealias(^^IntAlias) == (^^int) && (^^IntAlias) != (^^int));
static_assert(m::is_enumerator(^^Color::red) && m::is_bit_field(^^S::bf) && !m::is_bit_field(^^S::c));
static_assert(m::has_default_member_initializer(^^S::a) && !m::has_default_member_initializer(^^S::c));

consteval bool members_in_order() {
  auto all = m::nonstatic_data_members_of(^^S, unchecked);
  if (all.size() != 4) return false;
  if (all[0] != (^^S::a) || all[1] != (^^S::c) || all[2] != (^^S::bf) || all[3] != hidden_member()) return false;
  auto pub = m::nonstatic_data_members_of(^^S, m::access_context::unprivileged());
  if (pub.size() != 3) return false;
  auto st = m::static_data_members_of(^^S, unchecked);
  if (st.size() != 1 || st[0] != (^^S::sd)) return false;
  auto bases = m::bases_of(^^S, unchecked);
  if (bases.size() != 1 || m::type_of(bases[0]) != (^^Base) || !m::is_base(bases[0])) return false;
  auto en = m::enumerators_of(^^Color);
  if (en.size() != 3 || en[0] != (^^Color::red) || en[2] != (^^Color::blue)) return false;
  return true;
}
static_assert(members_in_order());

// layout
static_assert(m::offset_of(^^S::a).bytes == offsetof(S, a) && m::offset_of(^^S::a).bits == 0);
static_assert(m::offset_of(^^S::c).total_bits() == offsetof(S, c) * CHAR_BIT);
static_assert(m::size_of(^^S) == sizeof(S) && m::alignment_of(^^S) == alignof(S));
static_assert(m::size_of(^^S::c) == 1 && m::size_of(^^int) == sizeof(int));
static_assert(m::bit_size_of(^^S::bf) == 3 && m::bit_size_of(^^S::a) == sizeof(int) * CHAR_BIT);
static_assert(m::member_offset{1, 2}.total_bits() == CHAR_BIT + 2);
static_assert(m::member_offset{1, 2} < m::member_offset{1, 3});

// extract / reflect_constant / splice
static_assert(m::extract<Color>(^^Color::green) == Color::green);
static_assert(m::extract<int>(m::reflect_constant(42)) == 42);
static_assert(m::type_of(m::reflect_constant(42)) == (^^int));
static_assert([:m::reflect_constant(7):] == 7);
static_assert(std::is_same_v<typename[:^^S:], S>);
constexpr Base b_obj{5};
static_assert(b_obj.[:^^Base::b:] == 5);
static_assert(m::extract<int Base::*>(^^Base::b) == &Base::b);

// substitute
static_assert(m::can_substitute(^^Arr, {(^^int), m::reflect_constant(3)}));
static_assert(m::substitute(^^Arr, {(^^int), m::reflect_constant(3)}) == (^^Arr<int, 3>));
static_assert(!m::can_substitute(^^Arr, {(^^int)}));
static_assert(m::template_of(^^Arr<char, 2>) == (^^Arr));
static_assert(m::template_arguments_of(^^Arr<char, 2>)[0] == (^^char));

// type queries
static_assert(m::is_integral_type(^^int) && !m::is_integral_type(^^double));
static_assert(m::remove_const(^^const int) == (^^int) && m::add_pointer(^^int) == (^^int*));

// exceptions
consteval bool throws_on_bad_query() {
  try {
    (void)m::identifier_of(^^int);  // no identifier
  } catch (const m::exception&) {
    return true;
  }
  return false;
}
static_assert(throws_on_bad_query());

int main() {}
