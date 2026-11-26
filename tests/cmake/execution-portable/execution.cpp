#include <ycxx/execution/execution.hpp>
#include <ycxx/execution/stack_thread.hpp>
#include <tuple>
#include "check.hpp"

namespace ex = ycxx::execution;

static ex::task<int> compute(ex::parallel_scheduler scheduler) {
  const int left = co_await (ex::schedule(scheduler) | ex::then([] { return 20; }));
  const int right = co_await ex::just(22);
  co_return left + right;
}

int main() {
  ex::worker_thread worker;
  bool started = false;
  ex::start_worker(worker, 1u << 20, [&] { started = true; });
  worker.join();
  CHECK(started);

  auto scheduler = ex::get_parallel_scheduler();
  auto result = ex::sync_wait(compute(scheduler));
  CHECK(result && std::get<0>(*result) == 42);

  ex::counting_scope scope;
  auto future = ex::spawn_future(ex::schedule(scheduler) | ex::then([] { return 42; }), scope.get_token());
  auto answer = ex::sync_wait(std::move(future));
  scope.close();
  ex::sync_wait(scope.join());
  CHECK(answer && std::get<0>(*answer) == 42);
}
