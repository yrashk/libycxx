// As algorithm/no_extra_memory, for bidirectional iterators and for proxy iterators: the
// algorithms whose complexity depends on whether "enough extra memory is available" must still
// produce their specified, stable results when no memory can be obtained.
// [alg.partitions]/11-13 (stable_partition, BidirectionalIterator; ranges: bidirectional_iterator):
// "Effects: Places all the elements e in [first, last) that satisfy E(e) before all the
// elements that do not. The relative order of the elements in both groups is preserved."
// "Complexity: Let N = last - first: At most N log2 N swaps, but only O(N) swaps if there is
// enough extra memory. Exactly N applications of the predicate and projection."
// [alg.merge]/8-12 (inplace_merge, BidirectionalIterator): "Merges two sorted consecutive ranges
// ... into the range [first, last)"; "Remarks: Stable"; complexity with or without memory.
// [alg.sort] stable_sort for ranges::stable_sort over views::zip ([range.zip.iterator]: a proxy
// reference; the temporary storage would hold tuple values) -- "Remarks: Stable".
// Memory is refused by replacing the global allocation functions ([new.delete.single]) to throw
// bad_alloc while armed; the containers are built before arming. Many duplicate keys.
// UNSUPPORTED-SANITIZER: asan  ASan replaces the global allocation functions: its operator new neither calls the new_handler nor throws for impossible sizes, and its other forms do not forward to a program's replacement ([new.delete])
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <list>
#include <new>
#include <ranges>
#include <vector>
#include "check.hpp"

bool armed = false;

void* operator new(std::size_t n) {
  if (armed) throw std::bad_alloc();
  if (void* p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}
void* operator new(std::size_t n, std::align_val_t al) {
  if (armed) throw std::bad_alloc();
  std::size_t a = static_cast<std::size_t>(al);
  if (void* p = std::aligned_alloc(a, (n + a - 1) / a * a)) return p;
  throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }

struct KV {
  int key, id;
};

static bool stable_by_key(const std::list<KV>& l) {
  const KV* prev = nullptr;
  for (const KV& x : l) {
    if (prev && (x.key < prev->key || (x.key == prev->key && x.id < prev->id))) return false;
    prev = &x;
  }
  return true;
}

int main() {
  const int N = 2000;
  for (int mod : {1, 2, 3, 7, 50, 1000}) {
    std::vector<KV> src;
    unsigned s = 12345;
    for (int i = 0; i < N; ++i) {
      s = s * 1103515245u + 12345u;
      src.push_back(KV{static_cast<int>((s >> 16) % mod), i});
    }
    {  // std::stable_partition, bidirectional
      std::list<KV> l(src.begin(), src.end());
      long calls = 0;
      armed = true;
      auto mid = std::stable_partition(l.begin(), l.end(), [&](const KV& x) {
        ++calls;
        return x.key % 2 == 0;
      });
      armed = false;
      CHECK(calls == N);
      bool in_first = true;
      int last_true = -1, last_false = -1;
      for (auto it = l.begin(); it != l.end(); ++it) {
        if (it == mid) in_first = false;
        CHECK((it->key % 2 == 0) == in_first);
        int& last = in_first ? last_true : last_false;
        CHECK(it->id > last);
        last = it->id;
      }
    }
    {  // ranges::stable_partition, bidirectional, with a projection
      std::list<KV> l(src.begin(), src.end());
      long calls = 0;
      armed = true;
      auto r = std::ranges::stable_partition(l, [&](int k) { ++calls; return k % 3 == 1; }, &KV::key);
      armed = false;
      CHECK(calls == N && r.end() == l.end());
      bool in_first = true;
      int last_true = -1, last_false = -1;
      for (auto it = l.begin(); it != l.end(); ++it) {
        if (it == r.begin()) in_first = false;
        CHECK((it->key % 3 == 1) == in_first);
        int& last = in_first ? last_true : last_false;
        CHECK(it->id > last);
        last = it->id;
      }
    }
    {  // std::inplace_merge / ranges::inplace_merge, bidirectional
      for (int cut : {0, 1, N / 3, N / 2, N - 1, N}) {
        std::vector<KV> a(src.begin(), src.begin() + cut), b(src.begin() + cut, src.end());
        auto less = [](const KV& x, const KV& y) { return x.key < y.key; };
        std::stable_sort(a.begin(), a.end(), less);
        std::stable_sort(b.begin(), b.end(), less);
        std::list<KV> l(a.begin(), a.end());
        l.insert(l.end(), b.begin(), b.end());
        std::list<KV> l2 = l;
        auto mid = std::next(l.begin(), cut);
        auto mid2 = std::next(l2.begin(), cut);
        armed = true;
        std::inplace_merge(l.begin(), mid, l.end(), less);
        auto r = std::ranges::inplace_merge(l2, mid2, {}, &KV::key);
        armed = false;
        CHECK(r == l2.end());
        CHECK(stable_by_key(l) && stable_by_key(l2));
        CHECK(l.size() == static_cast<std::size_t>(N));
      }
    }
    {  // ranges::stable_sort / stable_partition / inplace_merge on a zip of proxies
      std::vector<int> keys, ids;
      for (const KV& x : src) {
        keys.push_back(x.key);
        ids.push_back(x.id);
      }
      auto z = std::views::zip(keys, ids);
      const auto key = [](const auto& t) -> int { return std::get<0>(t); };
      armed = true;
      std::ranges::stable_sort(z, {}, key);
      armed = false;
      for (int i = 1; i < N; ++i)
        CHECK(keys[i - 1] < keys[i] || (keys[i - 1] == keys[i] && ids[i - 1] < ids[i]));
      armed = true;
      auto p = std::ranges::stable_partition(z, [](int k) { return k % 2 == 1; }, key);
      armed = false;
      for (int i = 0; i < N; ++i) CHECK((keys[i] % 2 == 1) == (i < p.begin() - z.begin()));
      for (int i = 1; i < N; ++i)
        if ((keys[i - 1] % 2) == (keys[i] % 2))
          CHECK(keys[i - 1] < keys[i] || (keys[i - 1] == keys[i] && ids[i - 1] < ids[i]));
      armed = true;
      std::ranges::inplace_merge(z, p.begin(), {}, key);
      armed = false;
      for (int i = 1; i < N; ++i)
        CHECK(keys[i - 1] < keys[i] || (keys[i - 1] == keys[i] && ids[i - 1] < ids[i]));
    }
  }
  return 0;
}
