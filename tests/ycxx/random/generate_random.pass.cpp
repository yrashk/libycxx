// [alg.rand.generate]: ranges::generate_random(r, g) calls g.generate_random(r) if well-formed,
// otherwise fills r with values of g() (Remarks: equivalent to ranges::generate(r, ref(g)));
// generate_random(r, g, d) likewise with d.generate_random(r, g) or invoke(d, g); the iterator
// forms are equivalent to the range forms; all return the end (ranges::end(r)); constexpr.
#include <random>
#include <array>
#include <cstdint>
#include <ranges>
#include <span>
#include <type_traits>
#include "check.hpp"

struct counter_urbg {  // a constexpr-usable generator: 1, 2, 3, ...
  using result_type = std::uint32_t;
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return 0xffffffffu; }
  result_type n = 0;
  constexpr result_type operator()() { return ++n; }
};

struct bulk_urbg : counter_urbg {  // has a generate_random member: it must be used
  int bulk_calls = 0;
  template <class R> constexpr void generate_random(R&& r) {
    ++bulk_calls;
    for (auto& x : r) x = 1000 + (*this)();
  }
};

struct doubling {  // a distribution-like function object
  constexpr std::uint32_t operator()(counter_urbg& g) const { return 2 * g(); }
};
struct bulk_doubling : doubling {
  mutable int bulk_calls = 0;
  template <class R, class G> constexpr void generate_random(R&& r, G& g) const {
    ++bulk_calls;
    for (auto& x : r) x = 5000 + g();
  }
};

constexpr bool test() {
  {
    std::array<std::uint32_t, 5> a{};
    counter_urbg g;
    auto it = std::ranges::generate_random(a, g);
    if (it != a.end()) return false;
    for (std::uint32_t i = 0; i < 5; ++i)
      if (a[i] != i + 1) return false;
    if (g.n != 5) return false;
    // Iterator/sentinel form.
    auto e = std::ranges::generate_random(a.begin() + 1, a.begin() + 3, g);
    if (e != a.begin() + 3 || a[1] != 6 || a[2] != 7 || a[3] != 4) return false;
  }
  {
    std::array<std::uint32_t, 3> a{};
    bulk_urbg g;
    std::ranges::generate_random(a, g);
    if (g.bulk_calls != 1 || a[0] != 1001 || a[2] != 1003) return false;
  }
  {
    std::array<std::uint32_t, 4> a{};
    counter_urbg g;
    doubling d;
    auto it = std::ranges::generate_random(a, g, d);
    if (it != a.end() || a[0] != 2 || a[3] != 8) return false;
    bulk_doubling bd;
    std::ranges::generate_random(a, g, bd);
    if (bd.bulk_calls != 1 || a[0] != 5005) return false;
    auto e = std::ranges::generate_random(a.begin(), a.begin() + 2, g, d);
    if (e != a.begin() + 2) return false;
  }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  // With a real engine and distribution: the engine form gives the engine's own sequence.
  std::array<std::uint32_t, 64> a{};
  std::mt19937 g(5), ref(5);
  std::ranges::generate_random(a, g);
  for (auto v : a) CHECK(v == ref());
  CHECK(g == ref);
  std::array<int, 1000> b{};
  std::uniform_int_distribution<> d(-2, 2);
  std::ranges::generate_random(b, g, d);
  for (int v : b) CHECK(-2 <= v && v <= 2);
  // A span and a non-common range.
  std::span<std::uint32_t> s(a.data(), 10);
  CHECK(std::ranges::generate_random(s, g) == s.end());
  // An rvalue non-borrowed range gives ranges::dangling.
  static_assert(std::is_same_v<decltype(std::ranges::generate_random(std::array<std::uint32_t, 2>{}, g)),
                               std::ranges::dangling>);
}
