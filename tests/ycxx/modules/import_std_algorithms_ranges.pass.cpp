// [std.modules]/2: `import std;` provides <algorithm>, <numeric>, <ranges> (std::ranges, its
// customization point objects and std::views), <iterator>, <execution> and <functional>.
// MODULES: std
import std;
#include "module_check.hpp"

int main() {
  std::vector<int> v{5, 3, 1, 4, 2};
  std::sort(v.begin(), v.end());
  CHECK(std::is_sorted(v.begin(), v.end()) && std::binary_search(v.begin(), v.end(), 4));
  CHECK(std::accumulate(v.begin(), v.end(), 0) == 15 && std::reduce(std::execution::seq, v.begin(), v.end()) == 15);
  std::ranges::reverse(v);
  CHECK(v.front() == 5 && std::ranges::max(v) == 5 && std::ranges::contains(v, 3));
  auto even = v | std::views::filter([](int x) { return x % 2 == 0; }) | std::views::transform([](int x) { return x * 10; });
  CHECK(std::ranges::to<std::vector>(even) == std::vector<int>({40, 20}));
  CHECK(std::ranges::distance(std::views::iota(0, 10)) == 10);
  CHECK(std::ranges::size(std::ranges::views::take(v, 2)) == 2);
  auto [mn, mx] = std::ranges::minmax(v);
  CHECK(mn == 1 && mx == 5);
  std::vector<int> out;
  std::ranges::copy(std::views::reverse(v), std::back_inserter(out));
  CHECK(out.front() == 1);
  CHECK(std::ranges::begin(out) != std::ranges::end(out) && std::ranges::empty(std::views::empty<int>));
  int n = 0;
  for (auto [i, x] : std::views::enumerate(std::views::repeat(7, 3)))
    n += static_cast<int>(i) + x;
  CHECK(n == 24);
  auto words = std::string_view("a,bb,ccc") | std::views::split(',') | std::ranges::to<std::vector<std::string>>();
  CHECK(words.size() == 3 && words[2] == "ccc");
  CHECK(std::ranges::fold_left(std::views::iota(1, 5), 0, std::plus<>()) == 10);
  std::function<int(int)> f = std::bind_front(std::multiplies<int>(), 3);
  CHECK(f(4) == 12 && std::invoke(f, 1) == 3);
  std::move_only_function<int()> mf = [] { return 1; };
  CHECK(mf() == 1);
  CHECK(std::gcd(12, 18) == 6 && std::midpoint(1, 3) == 2);
  std::vector<int> iv(4);
  std::iota(iv.begin(), iv.end(), 1);
  CHECK(std::inner_product(iv.begin(), iv.end(), iv.begin(), 0) == 30);
  CHECK(std::distance(iv.begin(), std::next(iv.begin(), 2)) == 2);
  static_assert(std::random_access_iterator<std::vector<int>::iterator>);
  static_assert(std::ranges::contiguous_range<std::vector<int>>);
  static_assert(std::ranges::view<std::ranges::iota_view<int, int>>);
  return 0;
}
