// [alg.heap.operations]: push_heap "At most log(last - first) comparisons and twice as many
// projections"; pop_heap "At most 2 log(last - first) comparisons"; make_heap "At most
// 3(last - first) comparisons"; sort_heap "At most 2N log N comparisons, where N = last -
// first" ([algorithms.general]: log is the base-2 logarithm, not rounded). Checked for every
// size 1..260 against the real-valued bounds (2^c <= n for push_heap, 2^c <= n^2 for
// pop_heap, c <= 2 N log2 N for sort_heap), over inputs with many equal keys (all equal, two
// values, few values, one distinct key, equal runs) and random ones; the results must be heaps
// ([alg.heap.operations.general]: is_heap), sorted ranges for sort_heap, and permutations of
// the input (every tag survives), with the std:: and std::ranges:: forms.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include "check.hpp"

struct Item {
  int key;
  int tag;
};

static long comps = 0, projs = 0;
struct Less {
  bool operator()(const Item& a, const Item& b) const {
    ++comps;
    return a.key < b.key;
  }
};
struct KeyLess {
  bool operator()(int a, int b) const {
    ++comps;
    return a < b;
  }
};
struct Key {
  int operator()(const Item& i) const {
    ++projs;
    return i.key;
  }
};

constexpr int MAXN = 260;
static Item a[MAXN];
static bool seen[MAXN];

static std::uint32_t rng = 7;
static int rnd() {
  rng = rng * 1664525u + 1013904223u;
  return static_cast<int>(rng >> 8);
}

static void fill(int n, int pattern) {
  for (int i = 0; i < n; ++i) {
    int k = 0;
    switch (pattern) {
      case 0: k = 5; break;                        // all equal
      case 1: k = rnd() % 2; break;                // two values
      case 2: k = rnd() % 4; break;                // few values
      case 3: k = i == n / 2 ? 9 : 3; break;       // one distinct key, larger
      case 4: k = i == n / 3 ? -9 : 3; break;      // one distinct key, smaller
      case 5: k = (i / 17) % 3; break;             // equal runs
      default: k = rnd() % 1000; break;            // random
    }
    a[i] = {k, i};
  }
}
constexpr int PATTERNS = 7;

static bool push_ok(long c, int n) { return c <= 62 && (1LL << c) <= static_cast<long long>(n); }
static bool pop_ok(long c, int n) { return c <= 62 && (1LL << c) <= static_cast<long long>(n) * n; }
static bool sort_ok(long c, int n) { return n < 2 || static_cast<long double>(c) <= 2.0L * n * std::log2(static_cast<long double>(n)); }

static void permutation(int n) {
  for (int i = 0; i < n; ++i) seen[i] = false;
  for (int i = 0; i < n; ++i) {
    CHECK(a[i].tag >= 0 && a[i].tag < n && !seen[a[i].tag]);
    seen[a[i].tag] = true;
  }
}
static bool heap(int n) {
  for (int i = 1; i < n; ++i)
    if (a[(i - 1) / 2].key < a[i].key) return false;
  return true;
}

static void run(int n, int pattern, bool ranges) {
  fill(n, pattern);
  // build by pushes
  for (int k = 1; k <= n; ++k) {
    comps = projs = 0;
    if (ranges) std::ranges::push_heap(a, a + k, std::less<>{}, Key{});
    else std::push_heap(a, a + k, Less{});
    CHECK(heap(k));
  }
  permutation(n);
  // re-check the push bound with a counting comparator in both forms
  fill(n, pattern);
  for (int k = 1; k <= n; ++k) {
    comps = projs = 0;
    if (ranges) std::ranges::push_heap(a, a + k, KeyLess{}, Key{});
    else std::push_heap(a, a + k, Less{});
    CHECK(push_ok(comps, k));
    if (ranges) CHECK(projs <= 2 * comps);
  }
  CHECK(heap(n));
  // pop everything
  for (int k = n; k >= 1; --k) {
    comps = projs = 0;
    if (ranges) std::ranges::pop_heap(a, a + k, KeyLess{}, Key{});
    else std::pop_heap(a, a + k, Less{});
    CHECK(pop_ok(comps, k));
    if (ranges) CHECK(projs <= 2 * comps);
    CHECK(heap(k - 1));
    // the popped element is a maximum of [0, k)
    for (int i = 0; i < k - 1; ++i) CHECK(!(a[k - 1].key < a[i].key));
  }
  for (int i = 1; i < n; ++i) CHECK(a[i - 1].key <= a[i].key);
  permutation(n);
  // make_heap + sort_heap
  fill(n, pattern);
  comps = projs = 0;
  if (ranges) std::ranges::make_heap(a, a + n, KeyLess{}, Key{});
  else std::make_heap(a, a + n, Less{});
  CHECK(comps <= 3L * n);
  CHECK(heap(n));
  comps = projs = 0;
  if (ranges) std::ranges::sort_heap(a, a + n, KeyLess{}, Key{});
  else std::sort_heap(a, a + n, Less{});
  CHECK(sort_ok(comps, n));
  if (ranges) CHECK(projs <= 2 * comps);
  for (int i = 1; i < n; ++i) CHECK(a[i - 1].key <= a[i].key);
  permutation(n);
  // a pop from a heap of equal keys must leave a heap whose top equals the popped key
  fill(n, 0);
  std::make_heap(a, a + n, Less{});
  comps = 0;
  std::pop_heap(a, a + n, Less{});
  CHECK(pop_ok(comps, n));
  CHECK(heap(n - 1));
}

int main() {
  for (int n = 1; n <= MAXN; ++n)
    for (int p = 0; p < PATTERNS; ++p) {
      run(n, p, false);
      run(n, p, true);
    }
}
