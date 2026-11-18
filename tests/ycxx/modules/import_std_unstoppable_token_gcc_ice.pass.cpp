// [stoptoken.concepts] (unstoppable_token) and the parts of <execution> that check it, through
// `import std;`: when_all on the parallel scheduler ([exec.when.all], [exec.par.scheduler]),
// counting_scope with spawn ([exec.counting.scopes], [exec.spawn]), a stop token given through
// write_env ([exec.write.env]), and task awaiting senders ([exec.task]). Regression test for a
// GCC 16 modules ICE (tsubst_expr) on the draft's spelling of unstoppable_token (COMPILER-BUG(gcc)
// in stop_token.hpp; yrashk/libycxx#4).
// MODULES: std
// FLAGS: -pthread
// REQUIRES: exceptions
import std;
#include "module_check.hpp"

namespace ex = std::execution;

// A stoppable token whose stop_possible() is a constant false, through a non-static member.
struct never_member {
  template <class F>
  struct cb {
    cb(never_member, F) noexcept {}
  };
  template <class F>
  using callback_type = cb<F>;
  constexpr bool stop_requested() const noexcept { return false; }
  constexpr bool stop_possible() const noexcept { return false; }
  bool operator==(const never_member&) const = default;
};

// The same, but stop_possible() is not a constant expression.
struct runtime_member {
  template <class F>
  struct cb {
    cb(runtime_member, F) noexcept {}
  };
  template <class F>
  using callback_type = cb<F>;
  bool stop_requested() const noexcept { return false; }
  bool stop_possible() const noexcept { return false; }
  bool operator==(const runtime_member&) const = default;
};

static_assert(std::unstoppable_token<std::never_stop_token>);
static_assert(std::stoppable_token<std::stop_token> && !std::unstoppable_token<std::stop_token>);
static_assert(std::stoppable_token<std::inplace_stop_token> && !std::unstoppable_token<std::inplace_stop_token>);
static_assert(std::stoppable_token<never_member> && std::unstoppable_token<never_member>);
static_assert(std::stoppable_token<runtime_member> && !std::unstoppable_token<runtime_member>);
static_assert(!std::unstoppable_token<int>);

ex::task<int> add_on(ex::parallel_scheduler sch, int a, int b) {
  const int x = co_await (ex::schedule(sch) | ex::then([a] { return a; }));
  const int y = co_await ex::just(b);
  co_return x + y;
}

int main() {
  ex::parallel_scheduler sch = ex::get_parallel_scheduler();

  auto both = ex::when_all(ex::schedule(sch) | ex::then([] { return 1; }),
                           ex::schedule(sch) | ex::then([] { return 2; }));
  auto [a, b] = *std::this_thread::sync_wait(std::move(both));
  CHECK(a == 1 && b == 2);

  // spawn takes a sender without error completions ([exec.spawn])
  std::atomic<int> ran = 0;
  ex::counting_scope scope;
  for (int i = 0; i < 4; ++i) {
    auto work = ex::schedule(sch) | ex::then([&ran] { ran.fetch_add(1); }) |
                ex::upon_error([](std::exception_ptr) noexcept {});
    ex::spawn(std::move(work), scope.get_token());
  }
  std::this_thread::sync_wait(scope.join());
  CHECK(ran.load() == 4);

  std::stop_source src;
  auto seen = ex::write_env(ex::read_env(std::get_stop_token), ex::prop(std::get_stop_token, src.get_token()));
  auto [tok] = *std::this_thread::sync_wait(std::move(seen));
  CHECK(tok == src.get_token() && tok.stop_possible());

  auto [sum] = *std::this_thread::sync_wait(add_on(sch, 3, 4));
  CHECK(sum == 7);
}
