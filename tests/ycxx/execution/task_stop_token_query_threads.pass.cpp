// [task.state]/5.2, [task.promise]/12: the promise's environment answers get_stop_token with the
// token of the task's own stop source when the receiver's token is of another type. The
// environment reaches every agent the awaited operation runs on (a receiver's environment is
// queried from the threads completing it): the query must be safe to evaluate from several
// threads at once, with the same token each time. The source is made when the task starts, not by
// the first query (ThreadSanitizer sees the alternative).
// FLAGS: -pthread
// REQUIRES: exceptions
#include <execution>
#include <optional>
#include <stop_token>
#include <thread>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"
#include "watchdog.hpp"

namespace ex = std::execution;
using namespace exec_test;

// Queries the receiver's stop token on a new thread and on the starting thread at once, then
// completes with set_value().
struct two_queries {
  using sender_concept = ex::sender_tag;
  std::optional<std::inplace_stop_token>* here;
  std::optional<std::inplace_stop_token>* there;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    std::optional<std::inplace_stop_token>* here;
    std::optional<std::inplace_stop_token>* there;
    void start() & noexcept {
      std::thread t([this] { there->emplace(std::get_stop_token(ex::get_env(r))); });
      here->emplace(std::get_stop_token(ex::get_env(r)));
      t.join();
      ex::set_value(std::move(r));
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), here, there};
  }
};

struct inline_env {
  using start_scheduler_type = ex::inline_scheduler;
};
ex::task<void, inline_env> work(std::optional<std::inplace_stop_token>* here, std::optional<std::inplace_stop_token>* there) {
  co_await two_queries{here, there};
}

int main() {
  watchdog(20);
  for (int i = 0; i != 200; ++i) {
    std::stop_source src;
    std::optional<std::inplace_stop_token> here, there;
    record<int> rec;
    auto op = ex::connect(work(&here, &there), receiver_for(rec, ex::env{ex::prop(std::get_stop_token, src.get_token())}));
    ex::start(op);
    CHECK(rec.how == done::value && here && there && *here == *there && here->stop_possible() && !here->stop_requested());
  }
  return 0;
}
