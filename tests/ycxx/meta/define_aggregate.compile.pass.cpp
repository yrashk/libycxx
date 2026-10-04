// FLAGS: -freflection
// XFAIL-COMPILER: clang  Clang 23 has no reflection (P2996)
// [meta.reflection.define.aggregate]: data_member_spec(type, options) describes a data member
// (name, alignment, bit_width, no_unique_address); define_aggregate(^^C, specs) completes the
// incomplete class C with those non-static data members, in order, and returns ^^C.
// is_data_member_spec identifies the descriptions.
#include <meta>
#include <cstddef>
#include <type_traits>

namespace m = std::meta;

struct Point;
consteval {
  m::define_aggregate(^^Point, {m::data_member_spec(^^int, {.name = "x"}),
                                m::data_member_spec(^^double, {.name = "y"}),
                                m::data_member_spec(^^char, {.name = "tag", .alignment = 16})});
}
static_assert(m::is_complete_type(^^Point));
static_assert(std::is_aggregate_v<Point> && std::is_same_v<decltype(Point::x), int> &&
              std::is_same_v<decltype(Point::y), double>);
static_assert(offsetof(Point, tag) % 16 == 0 && alignof(Point) >= 16);
constexpr Point p{1, 2.5, 'q'};
static_assert(p.x == 1 && p.y == 2.5 && p.tag == 'q');
static_assert(m::is_data_member_spec(m::data_member_spec(^^int, {.name = "n"})) && !m::is_data_member_spec(^^int));

struct Bits;
consteval {
  m::define_aggregate(^^Bits, {m::data_member_spec(^^unsigned, {.name = "lo", .bit_width = 4}),
                               m::data_member_spec(^^unsigned, {.name = "hi", .bit_width = 4})});
}
static_assert(m::bit_size_of(^^Bits::lo) == 4 && m::is_bit_field(^^Bits::hi));

int main() {}
