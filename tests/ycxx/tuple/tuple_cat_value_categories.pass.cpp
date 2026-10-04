// [tuple.creation]/7-9: tuple_cat(tpls...) returns tuple<CTypes...>(celems...) where CTypes
// are the tuple_element_t<k, Ui> (Ui = remove_cvref_t<Ti>) and the element expressions are
// get<k>(std::forward<Ti>(tpi)). So element types are taken verbatim (references and cv kept,
// nested tuples not flattened), const sources are copied, rvalue sources are moved, and
// rvalue-reference elements stay rvalue references to the original objects; std::array
// arguments contribute N elements of their value_type.
#include <tuple>
#include <array>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Counted {
  static inline int copies = 0;
  static inline int moves = 0;
  int v;
  explicit Counted(int x) : v(x) {}
  Counted(const Counted& o) : v(o.v) { ++copies; }
  Counted(Counted&& o) noexcept : v(o.v) { ++moves; }
};

static_assert(std::is_same_v<decltype(std::tuple_cat(std::tuple<const int, std::tuple<long>>{}, std::array<char, 2>{})),
                             std::tuple<const int, std::tuple<long>, char, char>>);
static_assert(std::is_same_v<decltype(std::tuple_cat(std::forward_as_tuple(std::declval<int>()),
                                                     std::declval<const std::tuple<int&>&>())),
                             std::tuple<int&&, int&>>);
static_assert(std::is_same_v<decltype(std::tuple_cat(std::array<int, 0>{}, std::tuple<>{})), std::tuple<>>);

constexpr bool constexpr_test() {
  std::array<int, 3> a{1, 2, 3};
  auto t = std::tuple_cat(a, std::make_tuple(4L), std::array<int, 0>{});
  return std::get<0>(t) == 1 && std::get<2>(t) == 3 && std::get<3>(t) == 4L && std::tuple_size_v<decltype(t)> == 4;
}
static_assert(constexpr_test());

int main() {
  CHECK(constexpr_test());
  {
    const std::tuple<Counted> ct(Counted(1));
    std::tuple<Counted> mt(Counted(2));
    Counted::copies = Counted::moves = 0;
    auto r = std::tuple_cat(ct, std::move(mt));
    CHECK(std::get<0>(r).v == 1 && std::get<1>(r).v == 2);
    CHECK(Counted::copies == 1);  // from the const lvalue only
    CHECK(Counted::moves == 1);   // from the rvalue only
  }
  {
    std::array<Counted, 2> arr{Counted(3), Counted(4)};
    Counted::copies = Counted::moves = 0;
    auto r = std::tuple_cat(std::move(arr));
    CHECK(std::get<1>(r).v == 4);
    CHECK(Counted::copies == 0 && Counted::moves == 2);
    Counted::copies = Counted::moves = 0;
    auto r2 = std::tuple_cat(arr);
    (void)r2;
    CHECK(Counted::copies == 2 && Counted::moves == 0);
  }
  {
    Counted c(5);
    Counted::copies = Counted::moves = 0;
    auto r = std::tuple_cat(std::forward_as_tuple(std::move(c)), std::tie(c));
    static_assert(std::is_same_v<decltype(r), std::tuple<Counted&&, Counted&>>);
    CHECK(&std::get<0>(r) == &c && &std::get<1>(r) == &c);
    CHECK(Counted::copies == 0 && Counted::moves == 0);
  }
  return 0;
}
