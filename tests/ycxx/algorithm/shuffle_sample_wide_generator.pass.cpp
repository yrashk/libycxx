// [alg.random.shuffle]/2-3, [alg.random.sample]/3-5 with a uniform random bit generator whose range
// exceeds 64 bits ([rand.req.urng]: any unsigned integer result_type): shuffle permutes, sample
// selects the requested count, both terminate, and every position is equally likely (within a
// loose statistical bound). Ranges 2^65 + 1 (not a multiple of 2^64) and 2^128 (all values).
#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <numeric>
#include <random>
#include <ranges>
#include "check.hpp"

template <unsigned __int128 Max>
struct Wide {
  using result_type = unsigned __int128;
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return Max; }
  std::mt19937_64 e{12345};
  result_type operator()() {
    for (;;) {
      result_type v = (static_cast<result_type>(e()) << 64) | e();
      if (Max != ~static_cast<result_type>(0))
        v &= (static_cast<result_type>(1) << 66) - 1;  // uniform in [0, 2^66), then rejection
      if (v <= Max)
        return v;
    }
  }
};

template <class G>
void check(G g) {
  constexpr int n = 4, trials = 40000;
  int first[n] = {};
  for (int t = 0; t < trials; ++t) {
    std::array<int, n> a;
    std::iota(a.begin(), a.end(), 0);
    std::shuffle(a.begin(), a.end(), g);
    std::array<int, n> s = a;
    std::ranges::sort(s);
    CHECK(s == (std::array<int, n>{0, 1, 2, 3}));
    ++first[a[0]];
  }
  for (int k = 0; k < n; ++k)
    CHECK(first[k] > trials / n * 9 / 10 && first[k] < trials / n * 11 / 10);

  std::array<int, 10> pop;
  std::iota(pop.begin(), pop.end(), 0);
  std::array<int, 3> out{};
  auto e = std::sample(pop.begin(), pop.end(), out.begin(), 3, g);
  CHECK(e == out.end());
  CHECK(std::ranges::is_sorted(out));
  int chosen[10] = {};
  for (int t = 0; t < trials; ++t) {
    std::sample(pop.begin(), pop.end(), out.begin(), 3, g);
    for (int x : out)
      ++chosen[x];
  }
  for (int k = 0; k < 10; ++k)  // each element is chosen with probability 3/10
    CHECK(chosen[k] > trials * 3 / 10 * 9 / 10 && chosen[k] < trials * 3 / 10 * 11 / 10);
}

int main() {
  check(Wide<(static_cast<unsigned __int128>(1) << 65)>{});
  check(Wide<~static_cast<unsigned __int128>(0)>{});
  return 0;
}
