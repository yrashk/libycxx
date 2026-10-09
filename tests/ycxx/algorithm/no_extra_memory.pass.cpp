// libycxx allocation-fallback policy: these algorithms complete normally when allocation fails.
// This is stronger than conformance: [res.on.exception.handling]/4 permits bad_alloc to propagate
// from these potentially-throwing functions without a restrictive Throws paragraph.
// If an invocation completes, its results and complexity follow the clauses below:
// [stable.sort]/5: "If enough extra memory is available, N log(N) comparisons. Otherwise, at
// most N log^2(N) comparisons"; /6 "Remarks: Stable".
// [alg.merge]/11 (inplace_merge): "if enough additional memory is available, at most N - 1
// comparisons. Otherwise, O(N log N) comparisons"; /12 "Remarks: Stable".
// [alg.partitions]/13 (stable_partition): "at most N log2 N swaps, but only O(N) swaps if
// there is enough extra memory. Exactly N applications of the predicate and projection."
// The replaceable global allocation functions ([new.delete.single], [new.delete.array]) are
// replaced to throw bad_alloc while armed (the nothrow forms' default behavior calls them,
// [new.delete.single]/8), so every attempt to obtain memory fails. (Bounds are checked with
// log base 2 and generous constants.)
// UNSUPPORTED-SANITIZER: asan  ASan replaces the global allocation functions: its operator new neither calls the new_handler nor throws for impossible sizes, and its other forms do not forward to a program's replacement ([new.delete])
// REQUIRES: exceptions
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <new>
#include <ranges>
#include "sort_support.hpp"
#include "check.hpp"

bool armed = false;
int refused = 0;

void* operator new(std::size_t n) {
  if (armed) {
    ++refused;
    throw std::bad_alloc();
  }
  if (void* p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}
void* operator new(std::size_t n, std::align_val_t al) {
  if (armed) {
    ++refused;
    throw std::bad_alloc();
  }
  std::size_t a = static_cast<std::size_t>(al);
  if (void* p = std::aligned_alloc(a, (n + a - 1) / a * a)) return p;
  throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }

constexpr int N = 3000;
KV kv[N];

bool stable_sorted(const KV* a, int n) {
  for (int i = 1; i < n; ++i)
    if (a[i].key < a[i - 1].key || (a[i].key == a[i - 1].key && a[i].id < a[i - 1].id)) return false;
  return true;
}

struct CountingKVLess {
  long long* n;
  bool operator()(const KV& a, const KV& b) const {
    ++*n;
    return a.key < b.key;
  }
};

int main() {
  const int lg = ceil_log2(N);
  for (Pattern p : all_patterns) {
    int keys[N];
    fill_pattern(keys, N, p, 7);
    for (int i = 0; i < N; ++i) kv[i] = KV{keys[i] % 50, i};

    long long comps = 0;
    armed = true;
    std::stable_sort(kv, kv + N, CountingKVLess{&comps});
    armed = false;
    CHECK(stable_sorted(kv, N));
    CHECK(comps <= 1LL * N * lg * lg);

    for (int i = 0; i < N; ++i) kv[i] = KV{keys[i] % 50, i};
    comps = 0;
    armed = true;
    std::ranges::stable_sort(kv, CountingKVLess{&comps});
    armed = false;
    CHECK(stable_sorted(kv, N));
    CHECK(comps <= 1LL * N * lg * lg);

    // inplace_merge of two stably sorted halves
    for (int i = 0; i < N; ++i) kv[i] = KV{keys[i] % 50, i};
    std::stable_sort(kv, kv + N / 3);
    std::stable_sort(kv + N / 3, kv + N);
    comps = 0;
    armed = true;
    std::inplace_merge(kv, kv + N / 3, kv + N, CountingKVLess{&comps});
    armed = false;
    CHECK(stable_sorted(kv, N));
    CHECK(comps <= 4LL * N * lg);

    for (int i = 0; i < N; ++i) kv[i] = KV{keys[i] % 50, i};
    std::stable_sort(kv, kv + N / 2);
    std::stable_sort(kv + N / 2, kv + N);
    armed = true;
    CHECK(std::ranges::inplace_merge(kv, kv + N / 2, {}, &KV::key) == kv + N);
    armed = false;
    CHECK(stable_sorted(kv, N));

    // stable_partition: exactly N predicate applications, relative order kept
    for (int i = 0; i < N; ++i) kv[i] = KV{keys[i] % 50, i};
    int preds = 0;
    armed = true;
    KV* mid = std::stable_partition(kv, kv + N, [&](const KV& x) {
      ++preds;
      return x.key % 3 == 0;
    });
    armed = false;
    CHECK(preds == N);
    for (KV* q = kv; q != kv + N; ++q) CHECK((q < mid) == (q->key % 3 == 0));
    for (KV* q = kv + 1; q < mid; ++q) CHECK(q[-1].id < q->id);
    for (KV* q = mid + 1; q < kv + N; ++q) CHECK(q[-1].id < q->id);

    for (int i = 0; i < N; ++i) kv[i] = KV{keys[i] % 50, i};
    preds = 0;
    armed = true;
    auto sub = std::ranges::stable_partition(kv, [&](int k) {
      ++preds;
      return k % 2 == 1;
    }, &KV::key);
    armed = false;
    CHECK(preds == N);
    CHECK(sub.end() == kv + N);
    for (KV* q = kv; q != kv + N; ++q) CHECK((q < sub.begin()) == (q->key % 2 == 1));
    for (KV* q = kv + 1; q < sub.begin(); ++q) CHECK(q[-1].id < q->id);
    for (KV* q = sub.begin() + 1; q < kv + N; ++q) CHECK(q[-1].id < q->id);
  }
  return 0;
}
