// Nested bulk algorithms on the parallel scheduler, with a value datum:
//   [exec.par.scheduler]/10-12: bulk_chunked / bulk_unchunked of a child completing on the
//     parallel scheduler call the backend with a proxy whose execute runs f(i, j, args...) /
//     f(i, args...) with lvalues designating the child's values; other completions are forwarded;
//   [exec.bulk]: bulk_chunked and bulk_unchunked complete with the child's values, so an outer
//     bulk whose child is an inner bulk sees the same datum, and every index in [0, shape) is
//     covered exactly once per stage (chunks partition it).
// Both orders: bulk_chunked over bulk_unchunked and bulk_unchunked over bulk_chunked, plus bulk
// on top; the value 2 comes out through sync_wait.
// FLAGS: -pthread
#include <execution>
#include <atomic>
#include <memory>
#include <thread>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

constexpr int N = 97;

bool all_equal(const std::atomic<int>* hits, int v) {
  for (int i = 0; i < N; ++i)
    if (hits[i].load() != v)
      return false;
  return true;
}

int main() {
  auto par = ex::get_parallel_scheduler();
  {
    auto hits = std::make_unique<std::atomic<int>[]>(N);
    auto r = tt::sync_wait(ex::schedule(par) | ex::then([] { return 2; }) |
                           ex::bulk_unchunked(ex::par, N, [&](int i, int& v) { hits[i] += v; }) |
                           ex::bulk_chunked(ex::par, N, [&](int b, int e, int& v) {
                             for (int i = b; i < e; ++i)
                               hits[i] += 10 * v;
                           }) |
                           ex::bulk(ex::par, N, [&](int i, int& v) { hits[i] += 100 * v; }));
    CHECK(r && std::get<0>(*r) == 2);
    CHECK(all_equal(hits.get(), 222));
  }
  {
    auto hits = std::make_unique<std::atomic<int>[]>(N);
    auto r = tt::sync_wait(ex::schedule(par) | ex::then([] { return 2; }) |
                           ex::bulk_chunked(ex::par, N, [&](int b, int e, int& v) {
                             for (int i = b; i < e; ++i)
                               hits[i] += v;
                           }) |
                           ex::bulk_unchunked(ex::par, N, [&](int i, int& v) { hits[i] += 10 * v; }));
    CHECK(r && std::get<0>(*r) == 2);
    CHECK(all_equal(hits.get(), 22));
  }
  // seq policy (/10.3): one call covering everything per stage, value still passed through.
  {
    auto hits = std::make_unique<std::atomic<int>[]>(N);
    auto r = tt::sync_wait(ex::schedule(par) | ex::then([] { return 2; }) |
                           ex::bulk_chunked(ex::seq, N, [&](int b, int e, int& v) {
                             for (int i = b; i < e; ++i)
                               hits[i] += v;
                           }) |
                           ex::bulk_unchunked(ex::seq, N, [&](int i, int& v) { hits[i] += 10 * v; }));
    CHECK(r && std::get<0>(*r) == 2);
    CHECK(all_equal(hits.get(), 22));
  }
  return 0;
}
