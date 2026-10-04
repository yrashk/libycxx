// [tuple.like]/1: "A type T models and satisfies the exposition-only concept tuple-like if
// remove_cvref_t<T> is a specialization of array, complex, pair, tuple, or ranges::subrange."
// A program-defined type that implements the tuple protocol (tuple_size, tuple_element, get)
// is therefore not tuple-like. [tuple.syn]: apply and make_from_tuple take "tuple-like
// Tuple" and tuple_cat "tuple-like... Tuples" (constraints). [pairs.pair]/14-17 and
// [tuple.cnstr]: the pair-like / tuple-like converting constructors and assignments.
#include <tuple>
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

struct MyPair {
  int a = 0, b = 0;
  template <std::size_t I>
  constexpr int get() const { return I == 0 ? a : b; }
};
template <std::size_t I>
constexpr int get(const MyPair& p) { return p.template get<I>(); }
template <>
struct std::tuple_size<MyPair> : std::integral_constant<std::size_t, 2> {};
template <std::size_t I>
struct std::tuple_element<I, MyPair> { using type = int; };

// sanity: the protocol works for structured bindings
static_assert([] {
  auto [x, y] = MyPair{1, 2};
  return x == 1 && y == 2;
}());

// pair
static_assert(!std::is_constructible_v<std::pair<int, int>, MyPair>);
static_assert(!std::is_constructible_v<std::pair<int, int>, const MyPair&>);
static_assert(!std::is_assignable_v<std::pair<int, int>&, MyPair>);
static_assert(std::is_constructible_v<std::pair<int, int>, std::array<int, 2>>);
static_assert(std::is_assignable_v<std::pair<int, int>&, std::array<int, 2>>);
// tuple
static_assert(!std::is_constructible_v<std::tuple<int, int>, MyPair>);
static_assert(!std::is_assignable_v<std::tuple<int, int>&, MyPair>);
static_assert(std::is_constructible_v<std::tuple<int, int>, std::array<int, 2>>);
// apply / make_from_tuple / tuple_cat
auto add = [](int x, int y) { return x + y; };
template <class T>
concept applicable = requires(T t) { std::apply(add, t); };
template <class T>
concept makeable = requires(T t) { std::make_from_tuple<std::pair<int, int>>(t); };
template <class T>
concept catable = requires(T t) { std::tuple_cat(t); };
static_assert(!applicable<MyPair>);
static_assert(!makeable<MyPair>);
static_assert(!catable<MyPair>);
static_assert(applicable<std::array<int, 2>>);
static_assert(makeable<std::array<int, 2>>);
static_assert(catable<std::array<int, 2>>);
static_assert(applicable<std::pair<int, int>>);
static_assert(catable<const std::tuple<int, int>&>);
// and nothing else is tuple-like either
static_assert(!applicable<int>);
static_assert(!catable<int[2]>);
static_assert(std::is_same_v<decltype(std::tuple_cat(std::array<int, 2>{}, std::array<long, 1>{})),
                             std::tuple<int, int, long>>);
