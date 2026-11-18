// Nested bulk algorithms on a sender that completes on a task_scheduler, with a value datum:
//   [exec.task.scheduler]/1, /13.2: the completion domain of schedule(ts) is ts-domain;
//   /14-15: ts-domain's transform_sender turns bulk_chunked(child, par, shape, fn) and
//     bulk_unchunked(child, par, shape, fn) into senders that run fn over [0, shape) with lvalues
//     of decay-copies of the child's values, then complete with those values;
//   [exec.bulk]: bulk_chunked and bulk_unchunked complete with their child's values, so a bulk
//     whose child is another bulk (both on the task_scheduler's agent) sees the same datum.
// Every index is visited once by each stage, which both see the value 2.
// FLAGS: -pthread
#include <execution>
#include <thread>
#include <vector>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

int main() {
  ex::run_loop loop;
  std::thread worker([&] { loop.run(); });
  for (int pass = 0; pass < 2; ++pass) {
    ex::task_scheduler ts = pass == 0 ? ex::task_scheduler(ex::inline_scheduler{}) : ex::task_scheduler(loop.get_scheduler());
    std::vector<int> hits(29, 0);
    auto r = tt::sync_wait(ex::schedule(ts) | ex::then([] { return 2; }) |
                           ex::bulk_chunked(ex::par, 29, [&](int b, int e, int& v) {
                             for (int i = b; i < e; ++i)
                               hits[i] += v;
                           }) |
                           ex::bulk_unchunked(ex::par, 29, [&](int i, int& v) { hits[i] += 10 * v; }) |
                           ex::bulk(ex::par, 29, [&](int i, int& v) { hits[i] += 100 * v; }));
    CHECK(r && std::get<0>(*r) == 2);
    for (int h : hits)
      CHECK(h == 222);
  }
  loop.finish();
  worker.join();
  return 0;
}
