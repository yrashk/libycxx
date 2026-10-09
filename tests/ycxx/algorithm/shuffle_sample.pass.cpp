// Statistical smoke test: finite sample moments, frequencies, tails and observed coverage
// use chosen tolerances, not deterministic specification guarantees. Fixed seeds reproduce
// one implementation; sampling strategies can differ across implementations.
// Outlier estimates assume independent ideal draws; moment tolerances use normal/large-sample
// approximations where applicable. No universal or family-wide false-positive rate is claimed.
// Retained as a user-approved quality regression alongside independent deterministic checks.
// [alg.random.shuffle]: shuffle "Permutes the elements in the range [first, last)"; the
// result is a permutation; "Complexity: Exactly (last - first) - 1 swaps"; ranges::shuffle
// returns last; g is the source of randomness. [alg.random.sample]: "Copies min(last - first,
// n) elements (the sample) from [first, last) ... to out"; returns the end of the sample;
// "stable if and only if PopulationIterator models forward_iterator" (relative order kept).
// A hand-written uniform_random_bit_generator ([rand.req.urng]) keeps the test independent of
// <random>.
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <ranges>
#include "test_iterators.hpp"
#include "check.hpp"

struct Lcg {
  using result_type = std::uint32_t;
  std::uint64_t state;
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return 0xffffffffu; }
  result_type operator()() {
    state = state * 6364136223846793005ull + 1442695040888963407ull;
    return static_cast<result_type>(state >> 32);
  }
};
static_assert(std::uniform_random_bit_generator<Lcg>);

static bool is_perm_of_iota(const int* a, int n) {
  bool seen[64] = {};
  for (int i = 0; i < n; ++i) {
    if (a[i] < 0 || a[i] >= n || seen[a[i]]) return false;
    seen[a[i]] = true;
  }
  return true;
}

int main() {
  int a[50];
  for (int i = 0; i < 50; ++i) a[i] = i;
  Lcg g{12345};
  std::shuffle(a, a + 50, g);
  CHECK(is_perm_of_iota(a, 50));
  bool moved = false;
  for (int i = 0; i < 50; ++i) moved |= a[i] != i;
  CHECK(moved);  // a 50-element shuffle that is the identity would be astonishing

  // rvalue generator, empty and single-element ranges
  std::shuffle(a, a, Lcg{1});
  std::shuffle(a, a + 1, Lcg{1});
  CHECK(is_perm_of_iota(a, 50));

  int* rl = std::ranges::shuffle(a, g);
  CHECK(rl == a + 50);
  CHECK(is_perm_of_iota(a, 50));
  CHECK(std::ranges::shuffle(a, a + 10, g) == a + 10);

  // over many shuffles of 3 elements, every permutation shows up
  bool perms[3][3][3] = {};
  for (int t = 0; t < 600; ++t) {
    int s[3] = {0, 1, 2};
    std::shuffle(s, s + 3, g);
    CHECK(is_perm_of_iota(s, 3));
    perms[s[0]][s[1]][s[2]] = true;
  }
  int distinct = 0;
  for (auto& x : perms)
    for (auto& y : x)
      for (bool z : y) distinct += z;
  CHECK(distinct == 6);

  // sample: size min(n, N), stable for forward iterators, elements from the population
  int pop[20];
  for (int i = 0; i < 20; ++i) pop[i] = i * 3;
  int out[25] = {};
  int* e = std::sample(pop, pop + 20, out, 7, g);
  CHECK(e == out + 7);
  for (int i = 0; i < 7; ++i) {
    CHECK(0 <= out[i] && out[i] % 3 == 0 && out[i] < 60);
    if (i) CHECK(out[i - 1] < out[i]);  // stable: increasing like the population
  }
  e = std::sample(pop, pop + 20, out, 100, g);
  CHECK(e == out + 20);
  for (int i = 0; i < 20; ++i) CHECK(out[i] == pop[i]);
  CHECK(std::sample(pop, pop + 20, out, 0, g) == out);

  // input-iterator population into a random-access output
  int out2[5] = {};
  int* e2 = std::sample(InputIter<int>(pop), InputIter<int>(pop + 20), out2, 5L, g);
  CHECK(e2 == out2 + 5);
  bool selected[20] = {};
  for (int i = 0; i < 5; ++i) {
    CHECK(0 <= out2[i] && out2[i] < 60 && out2[i] % 3 == 0);
    CHECK(!selected[out2[i] / 3]);
    selected[out2[i] / 3] = true;
  }

  // ranges::sample
  int out3[4] = {};
  int* e3 = std::ranges::sample(pop, out3, 4, g);
  CHECK(e3 == out3 + 4);
  for (int i = 1; i < 4; ++i) CHECK(out3[i - 1] < out3[i]);
  int* e4 = std::ranges::sample(pop, pop + 3, out3, 10, g);
  CHECK(e4 == out3 + 3 && out3[2] == 6);

  // every element can be chosen
  bool chosen[20] = {};
  for (int t = 0; t < 200; ++t) {
    int one;
    std::sample(pop, pop + 20, &one, 1, g);
    CHECK(0 <= one && one < 60 && one % 3 == 0);
    chosen[one / 3] = true;
  }
  for (bool c : chosen) CHECK(c);
  return 0;
}
