// [pairs.pair]/18: pair(piecewise_construct_t, tuple<Args1...>, tuple<Args2...>): "Mandates:
// is_constructible_v<T1, Args1...> is true and is_constructible_v<T2, Args2...> is true."
#include <utility>
#include <tuple>

struct NeedsTwo {
  NeedsTwo(int, int) {}
};
std::pair<NeedsTwo, int> p(std::piecewise_construct, std::make_tuple(1), std::make_tuple(2));
