// [std.modules]/2: `import std;` exports every declaration of namespace std, so the reflection
// library of <meta> ([meta.syn]) is reachable through the module with no #include.
// [meta.syn]: std::meta::info is decltype(^^::); [meta.unary.cat]: is_reflection.
// [meta.reflection.names]: has_identifier, identifier_of. [meta.reflection.queries]: type_of,
//   parent_of. [meta.reflection.member.queries]: nonstatic_data_members_of (declaration order;
//   access_context::unchecked() includes private members, unprivileged() only public ones),
//   enumerators_of. [meta.reflection.layout]: offset_of, size_of. [meta.reflection.extract]:
//   extract. [expr.prim.splice], [basic.splice]: [: :]. [meta.define.static]: define_static_array,
//   define_static_string. [meta.reflection.exception]: an invalid query throws meta::exception.
// FLAGS: -freflection
// MODULES: std
// XFAIL: clang  Clang 23 has no reflection (P2996)
import std;
#include "module_check.hpp"

namespace m = std::meta;

struct point {
  int x;
  long y;
  char tag;
};
class account {
 public:
  int id;

 private:
  double balance;
};
enum class shade { light = 1, dark = 4 };
namespace geo {
constexpr int origin = 0;
}

static_assert(std::is_same_v<m::info, decltype(^^::)> && std::is_reflection_v<m::info> && !std::is_reflection_v<int>);
static_assert(m::has_identifier(^^point) && m::identifier_of(^^point) == "point" && !m::has_identifier(^^int));
static_assert(m::identifier_of(^^geo::origin) == "origin" && m::parent_of(^^geo::origin) == (^^geo));
static_assert(m::type_of(^^geo::origin) == (^^const int) && [:^^geo::origin:] == 0);
static_assert(m::type_of(^^point::y) == (^^long) && std::is_same_v<typename[:^^point:], point>);
static_assert(m::nonstatic_data_members_of(^^account, m::access_context::unchecked()).size() == 2);
static_assert(m::nonstatic_data_members_of(^^account, m::access_context::unprivileged()).size() == 1);
static_assert(m::enumerators_of(^^shade).size() == 2 && m::enumerators_of(^^shade)[1] == (^^shade::dark));
static_assert(m::extract<shade>(m::enumerators_of(^^shade)[1]) == shade::dark);
static_assert(m::size_of(^^point) == sizeof(point) && m::offset_of(^^point::x).bytes == 0);
static_assert(m::offset_of(^^point::y).bytes >= sizeof(int) && m::offset_of(^^point::tag).bytes > m::offset_of(^^point::y).bytes);

consteval std::vector<const char*> member_names(m::info cls) {
  std::vector<const char*> out;
  for (m::info mem : m::nonstatic_data_members_of(cls, m::access_context::unchecked()))
    out.push_back(std::define_static_string(m::identifier_of(mem)));
  return out;
}
constexpr auto point_names = std::define_static_array(member_names(^^point));
constexpr auto point_members = std::define_static_array(m::nonstatic_data_members_of(^^point, m::access_context::unchecked()));
static_assert(point_members.size() == 3 && point_members[0] == (^^point::x) && point_members[2] == (^^point::tag));

#if __cpp_exceptions
static_assert(std::derived_from<m::exception, std::exception>);
consteval bool bad_query_throws() {
  try {
    (void)m::identifier_of(^^int);  // int has no identifier
  } catch (const m::exception&) {
    return true;
  }
  return false;
}
static_assert(bad_query_throws());
#endif

int main() {
  std::vector<std::string_view> names(point_names.begin(), point_names.end());
  CHECK(names.size() == 3 && names[0] == "x" && names[1] == "y" && names[2] == "tag");
  point p{1, 2, 't'};
  p.[:point_members[1]:] = 40;
  CHECK(p.y == 40 && p.[:point_members[0]:] == 1 && p.[:point_members[2]:] == 't');
}
