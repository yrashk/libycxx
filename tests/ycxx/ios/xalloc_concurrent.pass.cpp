// [ios.base.storage]/1-2: xalloc() "Returns: index ++" (index: a static int) and "Concurrent
// access to this function by multiple threads does not result in a data race". So calls made
// before any thread exists and then by many threads at once return pairwise distinct values
// that, all together, form one run of consecutive integers. /4-/5: iword/pword on distinct
// stream objects from different threads, with those indices, keep their values (zero-
// initialized on first use).
// FLAGS: -pthread
#include <algorithm>
#include <atomic>
#include <sstream>
#include <thread>
#include <vector>
#include "check.hpp"

constexpr int K = 6;
constexpr int PER = 2000;

int main() {
  std::vector<int> before;
  for (int i = 0; i < 10; ++i) before.push_back(std::ios_base::xalloc());
  for (int i = 1; i < 10; ++i) CHECK(before[static_cast<std::size_t>(i)] == before[static_cast<std::size_t>(i) - 1] + 1);

  std::vector<std::vector<int>> got(K);
  std::atomic<bool> go{false};
  std::atomic<int> failures{0};
  std::vector<std::thread> ts;
  for (int k = 0; k < K; ++k)
    ts.emplace_back([&, k] {
      while (!go.load()) std::this_thread::yield();
      std::ostringstream s;
      for (int i = 0; i < PER; ++i) {
        int idx = std::ios_base::xalloc();
        got[static_cast<std::size_t>(k)].push_back(idx);
        if (i % 50 == 0) {
          if (s.iword(idx) != 0 || s.pword(idx) != nullptr) ++failures;  // new elements are zero
          s.iword(idx) = idx * 3L + k;
          s.pword(idx) = &got[static_cast<std::size_t>(k)];
        }
      }
      for (int i = 0; i < PER; i += 50) {
        int idx = got[static_cast<std::size_t>(k)][static_cast<std::size_t>(i)];
        if (s.iword(idx) != idx * 3L + k || s.pword(idx) != &got[static_cast<std::size_t>(k)]) ++failures;
      }
    });
  go = true;
  for (auto& t : ts) t.join();
  CHECK(failures.load() == 0);

  std::vector<int> all;
  for (auto& g : got) {
    for (std::size_t i = 1; i < g.size(); ++i) CHECK(g[i] > g[i - 1]);  // one thread's calls in order
    all.insert(all.end(), g.begin(), g.end());
  }
  std::sort(all.begin(), all.end());
  CHECK(all.size() == static_cast<std::size_t>(K * PER));
  CHECK(all.front() == before.back() + 1);
  for (std::size_t i = 1; i < all.size(); ++i) CHECK(all[i] == all[i - 1] + 1);  // distinct and contiguous
  CHECK(std::ios_base::xalloc() == all.back() + 1);
}
