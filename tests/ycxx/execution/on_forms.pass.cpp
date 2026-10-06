// [exec.on]/1-8, with sync_wait as the consumer ([exec.sync.wait]/2: sync-wait-env answers
// get_start_scheduler with its run_loop's scheduler, which runs on the waiting thread):
//   on(sch, sndr) (/1.1, /6, /7): sndr starts on sch's execution resource and, after it
//     completes, execution returns to the start scheduler (the waiting thread), with sndr's
//     result (values and errors alike);
//   on(sndr, sch, closure) and sndr | on(sch, closure) (/1.2, /6, /8): sndr runs where it is
//     started, the closure's work runs on sch's resource with sndr's result, then execution
//     returns to sndr's completion scheduler (here the inline start: the waiting thread).
//   /2: on(sch, sndr) needs a scheduler, and a sender or a closure.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <execution>
#include <thread>
#include <tuple>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

struct worker {
  ex::run_loop loop;
  std::thread t{[this] { loop.run(); }};
  ~worker() {
    loop.finish();
    t.join();
  }
  auto sch() { return loop.get_scheduler(); }
};

template <class A, class B>
concept on_ok = requires(A a, B b) { ex::on(a, b); };

int main() {
  const auto main_id = tt::get_id();
  worker w;
  const auto worker_id = w.t.get_id();

  static_assert(!on_ok<int, decltype(ex::just())>);
  static_assert(!on_ok<decltype(w.sch()), int>);
  static_assert(on_ok<decltype(w.sch()), decltype(ex::just())>);

  // on(sch, sndr)
  {
    auto s = ex::on(w.sch(), ex::just(5) | ex::then([](int v) { return std::pair(v, tt::get_id()); })) |
             ex::then([](std::pair<int, std::thread::id> p) { return std::tuple(p.first, p.second, tt::get_id()); });
    auto [r] = tt::sync_wait(std::move(s)).value();
    CHECK(std::get<0>(r) == 5 && std::get<1>(r) == worker_id && std::get<2>(r) == main_id);
  }
  // Errors come back too.
  {
    bool caught = false;
    try {
      tt::sync_wait(ex::on(w.sch(), ex::just(1) | ex::then([](int) -> int { throw 7; })));
    } catch (int e) {
      caught = e == 7;
    }
    CHECK(caught);
  }
  // on(sndr, sch, closure)
  {
    auto s2 = ex::on(ex::just(4), w.sch(), ex::then([](int v) { return std::pair(v * 2, tt::get_id()); })) |
              ex::then([](std::pair<int, std::thread::id> p) { return std::tuple(p.first, p.second, tt::get_id()); });
    auto [r] = tt::sync_wait(std::move(s2)).value();
    CHECK(std::get<0>(r) == 8 && std::get<1>(r) == worker_id && std::get<2>(r) == main_id);
    // The pipe form.
    auto s3 = ex::just(1) | ex::on(w.sch(), ex::then([](int v) { return std::pair(v, tt::get_id()); }));
    auto [p] = tt::sync_wait(std::move(s3)).value();
    CHECK(p.first == 1 && p.second == worker_id);
  }
  return 0;
}
