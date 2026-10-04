// [unord.req.general]/6: "In containers that support equivalent keys, elements with equivalent
// keys are adjacent to each other in the iteration order of the container."
// /9: "Rehashing invalidates iterators, changes ordering between elements, and changes which
// buckets elements appear in, but does not invalidate pointers or references to elements. For
// unordered_multiset and unordered_multimap, rehashing preserves the relative ordering of
// equivalent elements." rehash (/237), reserve (/239) and the automatic growth on insertion
// all rehash. /242: "The erase members shall invalidate only iterators and references to the
// erased elements, and preserve the relative order of the elements that are not erased."
// /244: extract likewise preserves the relative order of the elements not extracted.
// A deliberately poor hash (three hash codes for ten keys) puts several keys in a bucket.
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "check.hpp"

struct BadHash {
  std::size_t operator()(int x) const { return static_cast<std::size_t>(x % 3); }
};
using MM = std::unordered_multimap<int, int, BadHash>;

static std::vector<int> group(const MM& m, int k) {
  std::vector<int> r;
  auto [a, b] = m.equal_range(k);
  for (; a != b; ++a) r.push_back(a->second);
  return r;
}
static bool adjacent(const MM& m, int nkeys) {
  std::vector<int> runs;
  for (const auto& p : m)
    if (runs.empty() || runs.back() != p.first) runs.push_back(p.first);
  return static_cast<int>(runs.size()) == nkeys;
}
template <class V>
static bool subsequence(const V& small, const V& big) {
  auto it = big.begin();
  for (const auto& x : small) {
    it = std::find(it, big.end(), x);
    if (it == big.end()) return false;
    ++it;
  }
  return true;
}
static std::vector<std::pair<int, int>> items(const MM& m) { return {m.begin(), m.end()}; }

int main() {
  MM mm;
  for (int i = 0; i < 400; ++i) mm.emplace(i % 10, i);  // grows (and rehashes) as it goes
  CHECK(adjacent(mm, 10));
  std::vector<std::vector<int>> before;
  for (int k = 0; k < 10; ++k) before.push_back(group(mm, k));
  for (int k = 0; k < 10; ++k) CHECK(before[k].size() == 40);
  std::vector<const int*> addr;
  for (const auto& p : mm) addr.push_back(&p.second);
  auto same = [&] {
    for (int k = 0; k < 10; ++k)
      if (group(mm, k) != before[k]) return false;
    std::vector<const int*> now;
    for (const auto& p : mm) now.push_back(&p.second);
    std::sort(now.begin(), now.end());
    auto old = addr;
    std::sort(old.begin(), old.end());
    return now == old && adjacent(mm, 10);
  };
  mm.rehash(1000);
  CHECK(same());
  mm.rehash(1);
  CHECK(same());
  mm.reserve(5000);
  CHECK(same());
  mm.max_load_factor(100.0f);
  mm.rehash(0);
  CHECK(same());
  mm.max_load_factor(0.1f);
  mm.emplace(3, 1000);  // forces growth
  CHECK(group(mm, 3).size() == 41 && adjacent(mm, 10));
  for (int k = 0; k < 10; ++k)
    if (k != 3) CHECK(group(mm, k) == before[k]);
  auto g3 = group(mm, 3);
  CHECK(subsequence(before[3], g3));

  auto all = items(mm);
  std::erase_if(mm, [](const auto& p) { return p.second % 3 == 0; });
  CHECK(subsequence(items(mm), all) && adjacent(mm, 10));
  all = items(mm);
  mm.erase(mm.find(5));
  CHECK(subsequence(items(mm), all) && items(mm).size() + 1 == all.size());
  all = items(mm);
  CHECK(mm.erase(7) > 0);
  CHECK(subsequence(items(mm), all) && adjacent(mm, 9));
  all = items(mm);
  auto nh = mm.extract(2);
  CHECK(!nh.empty() && subsequence(items(mm), all) && items(mm).size() + 1 == all.size());

  std::unordered_multiset<int, BadHash> ms;
  for (int i = 0; i < 100; ++i) ms.insert(i % 4);
  ms.rehash(500);
  std::vector<int> seq(ms.begin(), ms.end());
  int runs = 0;
  for (std::size_t i = 0; i < seq.size(); ++i)
    if (i == 0 || seq[i] != seq[i - 1]) ++runs;
  CHECK(runs == 4);
  return 0;
}
