// [exec.parschedrepl]: query_parallel_scheduler_backend is replaceable; get_parallel_scheduler
// uses the program's backend. A backend completes the proxies it is given (here inline, on the
// calling thread); try_query<inplace_stop_token>(get_stop_token) is supported
// ([exec.parschedrepl.recvproxy]/4); a bulk proxy's execute runs the iterations.
#include <execution>
#include <atomic>
#include <memory>
#include <optional>
#include <span>
#include <stop_token>
#include "check.hpp"

namespace ex = std::execution;
namespace psr = ex::parallel_scheduler_replacement;

struct inline_backend : psr::parallel_scheduler_backend {
  static inline std::atomic<int> scheduled{0}, bulks{0};
  void schedule(psr::receiver_proxy& r, std::span<std::byte> s) noexcept override {
    ++scheduled;
    CHECK(s.size() > 0);
    auto tok = r.try_query<std::inplace_stop_token>(std::get_stop_token);
    CHECK(tok.has_value());
    if (tok->stop_requested())
      r.set_stopped();
    else
      r.set_value();
  }
  void schedule_bulk_chunked(std::size_t n, psr::bulk_item_receiver_proxy& r, std::span<std::byte>) noexcept override {
    ++bulks;
    if (n != 0)
      r.execute(0, n);
    r.set_value();
  }
  void schedule_bulk_unchunked(std::size_t n, psr::bulk_item_receiver_proxy& r, std::span<std::byte>) noexcept override {
    ++bulks;
    for (std::size_t i = 0; i < n; ++i)
      r.execute(i, i + 1);
    r.set_value();
  }
};

namespace std::execution::parallel_scheduler_replacement {
shared_ptr<parallel_scheduler_backend> query_parallel_scheduler_backend() {
  static auto b = make_shared<inline_backend>();
  return b;
}
} // namespace std::execution::parallel_scheduler_replacement

int main() {
  auto sch = ex::get_parallel_scheduler();
  auto r = std::this_thread::sync_wait(ex::schedule(sch) | ex::then([] { return 1; }));
  CHECK(r && std::get<0>(*r) == 1);
  CHECK(inline_backend::scheduled == 1);
  int sum = 0;
  auto rb = std::this_thread::sync_wait(ex::schedule(sch) | ex::bulk(ex::par, 10, [&](int i) { sum += i; }));
  CHECK(rb && sum == 45);
  CHECK(inline_backend::bulks == 1);
  return 0;
}
