// [intseq.binding]: tuple_size / tuple_element / get for integer_sequence, enabling
// structured bindings. "Mandates: I < sizeof...(Values)." "Returns: Values...[I]."
// tuple_element yields T for both integer_sequence and const integer_sequence.
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

using Seq = std::integer_sequence<int, 4, -2, 9>;
using USeq = std::integer_sequence<unsigned char, 255, 0>;
using Empty = std::index_sequence<>;

static_assert(std::tuple_size<Seq>::value == 3);
static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 3>, std::tuple_size<Seq>>);
static_assert(std::tuple_size_v<Seq> == 3);
static_assert(std::tuple_size_v<const Seq> == 3);        // via tuple_size<const T>
static_assert(std::tuple_size_v<USeq> == 2);
static_assert(std::tuple_size_v<Empty> == 0);
static_assert(std::tuple_size_v<std::make_index_sequence<7>> == 7);
static_assert(std::is_same_v<std::tuple_element_t<0, Seq>, int>);
static_assert(std::is_same_v<std::tuple_element_t<2, Seq>, int>);
static_assert(std::is_same_v<std::tuple_element_t<1, const Seq>, int>);   // T, not const T
static_assert(std::is_same_v<std::tuple_element_t<0, USeq>, unsigned char>);
static_assert(std::is_same_v<std::tuple_element_t<1, const USeq>, unsigned char>);

static_assert(std::get<0>(Seq{}) == 4);
static_assert(std::get<1>(Seq{}) == -2);
static_assert(std::get<2>(Seq{}) == 9);
static_assert(std::get<0>(USeq{}) == 255);
static_assert(std::get<3>(std::make_index_sequence<5>{}) == 3);
static_assert(std::is_same_v<decltype(std::get<0>(Seq{})), int>);        // returns T by value
static_assert(std::is_same_v<decltype(std::get<1>(USeq{})), unsigned char>);
static_assert(noexcept(std::get<0>(Seq{})));

constexpr bool test() {
  auto [a, b, c] = Seq{};
  if (a != 4 || b != -2 || c != 9) return false;
  const auto [x, y] = USeq{};
  if (x != 255 || y != 0) return false;
  auto& [p, q, r] = static_cast<const std::index_sequence<10, 20, 30>&>(std::index_sequence<10, 20, 30>{});
  if (p != 10 || q != 20 || r != 30) return false;
  static_assert(std::is_same_v<decltype(a), int>);
  // tuple_element<I, const integer_sequence<T, ...>>::type is T, so the referenced type is T.
  static_assert(std::is_same_v<decltype(x), unsigned char>);
  // Unqualified get is found through ADL ([dcl.struct.bind] uses get<i>(e) lookup).
  if (get<2>(Seq{}) != 9) return false;
  return true;
}
static_assert(test());

#if defined(__cpp_structured_bindings) && __cpp_structured_bindings >= 202411L
// Structured binding packs (P1061) combined with integer_sequence.
template <std::size_t N>
constexpr std::size_t sum_indices() {
  auto [... is] = std::make_index_sequence<N>{};
  return (std::size_t(0) + ... + is);
}
static_assert(sum_indices<5>() == 10);
static_assert(sum_indices<0>() == 0);
#endif

int main() {
  CHECK(test());
  auto [a, b, c] = Seq{};
  CHECK(a + b + c == 11);
  return 0;
}
