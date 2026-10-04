// [pairs.pair]/18-19: pair(piecewise_construct_t, tuple<Args1...> first_args, tuple<Args2...>
// second_args): "Initializes first with arguments of types Args1... obtained by forwarding the
// elements of first_args and initializes second with arguments of types Args2... obtained by
// forwarding the elements of second_args. (Here, forwarding an element x of type U within a
// tuple object means calling std::forward<U>(x).)" [pair.piecewise]: piecewise_construct_t has
// an explicit defaulted default constructor; piecewise_construct is an inline constexpr object.
// No copy or move of the element types is required.
#include <utility>
#include <tuple>
#include <type_traits>
#include "check.hpp"

struct Immovable {
  int a, b;
  constexpr Immovable(int x, int y) : a(x), b(y) {}
  Immovable(Immovable&&) = delete;
};
struct Kind {  // records how its argument arrived
  int k;
  constexpr Kind() : k(0) {}
  constexpr Kind(int&) : k(1) {}
  constexpr Kind(int&&) : k(2) {}
  constexpr Kind(const int&) : k(3) {}
};

static_assert(std::is_empty_v<std::piecewise_construct_t>);
static_assert(std::is_same_v<decltype(std::piecewise_construct), const std::piecewise_construct_t>);
template <class T>
concept copy_list_init_from_braces = requires { [](T) {}({}); };
static_assert(!copy_list_init_from_braces<std::piecewise_construct_t>);  // explicit default ctor

constexpr bool test() {
  std::pair<Immovable, Immovable> p(std::piecewise_construct, std::forward_as_tuple(1, 2),
                                    std::make_tuple(3, 4));
  if (p.first.a != 1 || p.first.b != 2 || p.second.a != 3 || p.second.b != 4) return false;
  // empty argument tuples value/default-initialize
  std::pair<Kind, int> e(std::piecewise_construct, std::tuple<>(), std::tuple<>());
  if (e.first.k != 0 || e.second != 0) return false;
  // forwarding preserves the tuple's element types
  int i = 0;
  const int ci = 0;
  std::pair<Kind, Kind> f1(std::piecewise_construct, std::forward_as_tuple(i),
                           std::forward_as_tuple(std::move(i)));
  if (f1.first.k != 1 || f1.second.k != 2) return false;
  std::pair<Kind, Kind> f2(std::piecewise_construct, std::tuple<const int&>(ci), std::tuple<int>(5));
  if (f2.first.k != 3 || f2.second.k != 2) return false;  // tuple<int>: forward<int> gives int&&
  // reference members
  int target = 7;
  std::pair<int&, const int&> refs(std::piecewise_construct, std::forward_as_tuple(target),
                                   std::forward_as_tuple(target));
  refs.first = 8;
  if (refs.second != 8) return false;
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
  return 0;
}
