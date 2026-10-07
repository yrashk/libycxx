// [ptrtag.pair.comp]/1: operator<=> "Effects: Equivalent to: return pair(lhs.pointer(),
// lhs.tag()) <=> pair(rhs.pointer(), rhs.tag());" requires three_way_comparable<tag_type>.
// /3: operator== "Equivalent to: return pair(lhs.pointer(), lhs.tag()) == pair(rhs.pointer(),
// rhs.tag());" requires equality_comparable<tag_type>. Both noexcept.
// /2, /4 (recommended practice): compare the representations when the tag's operator is built in;
// a user-declared operator of an enumeration tag must still be the one used.
// [memory.syn]: tuple_size, tuple_element (also of const pointer_tag_pair);
// [ptrtag.pair.get]/1-2: get<0> is p.pointer(), get<1> is p.tag(); noexcept.
// [ptrtag.pair.general]: the deduction guide pointer_tag_pair(Ptr*, TagT).
#include <compare>
#include <concepts>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace user {
// A tag whose ordering is reversed and whose equality ignores bit 1 (a user-declared operator).
enum class Rev : unsigned { a = 0, b = 1, c = 2, d = 3 };
inline int calls = 0;
constexpr std::weak_ordering operator<=>(Rev x, Rev y) {
  ++calls;
  return static_cast<unsigned>(y) <=> static_cast<unsigned>(x) == 0 ? std::weak_ordering::equivalent
         : static_cast<unsigned>(y) < static_cast<unsigned>(x)       ? std::weak_ordering::less
                                                                     : std::weak_ordering::greater;
}
constexpr bool operator==(Rev x, Rev y) {
  ++calls;
  return (static_cast<unsigned>(x) & 1u) == (static_cast<unsigned>(y) & 1u);
}
// Not comparable at all.
enum class NoCmp : unsigned { x };
bool operator==(NoCmp, NoCmp) = delete;
std::strong_ordering operator<=>(NoCmp, NoCmp) = delete;
} // namespace user

using PT = std::pointer_tag_pair<int*, 2>;
static_assert(std::is_same_v<decltype(std::declval<PT>() <=> std::declval<PT>()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::declval<PT>() == std::declval<PT>()), bool>);
static_assert(noexcept(std::declval<PT>() <=> std::declval<PT>()) && noexcept(std::declval<PT>() == std::declval<PT>()));
using PR = std::pointer_tag_pair<int*, 2, user::Rev>;
// The result is pair's: common_comparison_category of strong (pointer) and weak (tag).
static_assert(std::is_same_v<decltype(std::declval<PR>() <=> std::declval<PR>()), std::weak_ordering>);
using PN = std::pointer_tag_pair<int*, 2, user::NoCmp>;
static_assert(!std::three_way_comparable<PN>);
static_assert(!std::equality_comparable<PN>);
static_assert(std::three_way_comparable<PT> && std::equality_comparable<PT>);

// Tuple interface.
static_assert(std::tuple_size_v<PT> == 2 && std::tuple_size_v<const PT> == 2);
static_assert(std::is_same_v<std::tuple_element_t<0, PT>, int*>);
static_assert(std::is_same_v<std::tuple_element_t<1, PT>, unsigned>);
static_assert(std::is_same_v<std::tuple_element_t<0, const PT>, int*>); // not int* const
static_assert(std::is_same_v<std::tuple_element_t<1, const PT>, unsigned>);
static_assert(std::is_same_v<std::tuple_element_t<1, const PR>, user::Rev>);
static_assert(std::is_same_v<decltype(std::get<0>(std::declval<PT>())), int*>);
static_assert(std::is_same_v<decltype(std::get<1>(std::declval<PR>())), user::Rev>);
static_assert(noexcept(std::get<0>(std::declval<PT>())));

// Deduction guide: pointer_tag_pair(Ptr*, TagT) -> pointer_tag_pair<Ptr*, bits-available<Ptr>, TagT>
// (the draft's element-of<Ptr> is element-of<Ptr*>, see STATUS "Draft issues noticed").
struct alignas(8) Eight {
  char c;
};
static_assert(std::is_same_v<decltype(std::pointer_tag_pair(std::declval<Eight*>(), 1u)),
                             std::pointer_tag_pair<Eight*, 3, unsigned>>);
static_assert(std::is_same_v<decltype(std::pointer_tag_pair(std::declval<const int*>(), user::Rev::a)),
                             std::pointer_tag_pair<const int*, 2, user::Rev>>);

int main() {
  int arr[2] = {};
  int* lo = &arr[0];
  int* hi = &arr[1];
  // Ordered by pointer first, then by tag.
  CHECK((PT(lo, 3u) <=> PT(hi, 0u)) == std::strong_ordering::less);
  CHECK((PT(hi, 0u) <=> PT(lo, 3u)) == std::strong_ordering::greater);
  CHECK((PT(lo, 1u) <=> PT(lo, 2u)) == std::strong_ordering::less);
  CHECK((PT(lo, 2u) <=> PT(lo, 2u)) == std::strong_ordering::equal);
  CHECK(PT(lo, 1u) < PT(lo, 2u) && PT(lo, 3u) < PT(hi, 0u) && PT(hi, 0u) >= PT(lo, 3u));
  CHECK(PT(lo, 2u) == PT(lo, 2u));
  CHECK(PT(lo, 2u) != PT(lo, 1u));
  CHECK(PT(lo, 2u) != PT(hi, 2u));
  CHECK(PT() == PT(nullptr, 0u));
  CHECK(PT() != PT(nullptr, 1u));
  CHECK(PT(nullptr, 3u) < PT(lo, 0u));
  // Enumeration tag with the built-in operators.
  enum class E : unsigned { x, y };
  using PE = std::pointer_tag_pair<int*, 1, E>;
  CHECK(PE(lo, E::x) < PE(lo, E::y) && PE(lo, E::y) == PE(lo, E::y));
  // A user-declared operator is the one used ([ptrtag.pair.comp]/1, /3).
  user::calls = 0;
  CHECK((PR(lo, user::Rev::a) <=> PR(lo, user::Rev::d)) == std::weak_ordering::greater);
  CHECK((PR(lo, user::Rev::d) <=> PR(lo, user::Rev::a)) == std::weak_ordering::less);
  CHECK((PR(lo, user::Rev::d) <=> PR(hi, user::Rev::a)) == std::weak_ordering::less); // pointer first
  CHECK(PR(lo, user::Rev::a) == PR(lo, user::Rev::c));  // bit 1 ignored
  CHECK(!(PR(lo, user::Rev::a) == PR(lo, user::Rev::b)));
  CHECK(!(PR(lo, user::Rev::a) == PR(hi, user::Rev::a))); // pointers differ: tag not consulted
  CHECK(user::calls == 4);

  // get and structured bindings.
  PT p(hi, 2u);
  CHECK(std::get<0>(p) == hi && std::get<1>(p) == 2u);
  const PT cp(lo, 1u);
  CHECK(std::get<0>(cp) == lo && std::get<1>(cp) == 1u);
  auto [ptr, tag] = p;
  CHECK(ptr == hi && tag == 2u);
  const auto& [cptr, ctag] = cp;
  static_assert(std::is_same_v<decltype(cptr), int*>); // tuple_element<0, const PT> is Ptr
  CHECK(cptr == lo && ctag == 1u);

  // The deduced type works.
  Eight e{};
  auto d = std::pointer_tag_pair(&e, 7u);
  CHECK(d.pointer() == &e && d.tag() == 7u);
  return 0;
}
