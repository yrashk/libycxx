// associate ([exec.associate]) and the stop-when fusion of counting_scope's token
// ([exec.scope.counting], [exec.stop.when]):
//   [exec.associate]/2-3, /9.2: associate(sndr, token) wraps sndr with token.wrap and tries to
//     associate when it is created, and the sender owns that association; /5: copying the sender tries to associate again (the copy
//     is associated iff that succeeds); /7: destroying an associated sender that was never
//     started releases its association; an unassociated sender completes with set_stopped.
//   [exec.scope.counting]/7: counting_scope::token::wrap(snd) is stop-when(snd,
//     scope->s_source.get_token()); /3: request_stop() requests a stop on s_source.
//   [exec.stop.when]/2.3.1: connected to a receiver with an unstoppable token, the sender sees
//     the scope's token; /2.3.2: otherwise it sees a token whose stop_requested() is true when
//     either the scope's or the receiver's is, and whose callbacks run (once) when either is
//     requested.
//   [exec.simple.counting.token]/1: simple_counting_scope's wrap returns the sender unchanged, so
//     the sender sees the receiver's token.
// FLAGS: -pthread
#include <execution>
#include <optional>
#include <stop_token>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

using stoppable_env = ex::prop<std::get_stop_token_t, std::inplace_stop_token>;

// Sends whether a stop was requested of the token the operation sees.
inline auto stop_seen() {
  return ex::read_env(std::get_stop_token) | ex::then([](auto tok) noexcept { return tok.stop_requested(); });
}

int main() {
  // counting_scope: the scope's stop reaches the work, with or without a stoppable receiver
  {
    ex::counting_scope scope;
    std::optional s(ex::associate(stop_seen(), scope.get_token())); // owns an association
    record<int, bool> r1;
    run(*s, receiver_for(r1)); // env<>: never_stop_token
    CHECK(r1.how == done::value && std::get<0>(*r1.values) == false);
    scope.request_stop();
    record<int, bool> r2;
    run(*s, receiver_for(r2)); // a copy is connected (lvalue): associated again
    CHECK(r2.how == done::value && std::get<0>(*r2.values) == true);
    std::inplace_stop_source src;
    record<int, bool> r3;
    run(*s, receiver_for(r3, stoppable_env(std::get_stop_token, src.get_token())));
    CHECK(r3.how == done::value && std::get<0>(*r3.values) == true);
    s.reset(); // releases its association, so that the join can complete
    std::this_thread::sync_wait(scope.join());
  }
  // counting_scope: the receiver's stop reaches the work too (fused token)
  {
    ex::counting_scope scope;
    std::inplace_stop_source src;
    src.request_stop();
    record<int, bool> r;
    run(ex::associate(stop_seen(), scope.get_token()),
        receiver_for(r, stoppable_env(std::get_stop_token, src.get_token())));
    CHECK(r.how == done::value && std::get<0>(*r.values) == true);
    std::this_thread::sync_wait(scope.join());
  }
  // the fused token's callbacks: invoked once, whichever source requests first
  {
    ex::counting_scope scope;
    std::inplace_stop_source src;
    int calls = 0;
    auto body = ex::read_env(std::get_stop_token) | ex::then([&](auto tok) noexcept {
                  struct counter {
                    int* n;
                    void operator()() noexcept { ++*n; }
                  };
                  std::stop_callback_for_t<decltype(tok), counter> cb(tok, counter{&calls});
                  CHECK(calls == 0);
                  scope.request_stop();
                  CHECK(calls == 1);
                  src.request_stop();
                  CHECK(calls == 1);
                });
    record<int> r;
    run(ex::associate(std::move(body), scope.get_token()),
        receiver_for(r, stoppable_env(std::get_stop_token, src.get_token())));
    CHECK(r.how == done::value && calls == 1);
    std::this_thread::sync_wait(scope.join());
  }
  // simple_counting_scope: no fusion, the receiver's token as is
  {
    ex::simple_counting_scope scope;
    std::inplace_stop_source src;
    record<int, bool> r;
    std::optional s(ex::associate(stop_seen(), scope.get_token()));
    run(*s, receiver_for(r, stoppable_env(std::get_stop_token, src.get_token())));
    CHECK(r.how == done::value && std::get<0>(*r.values) == false);
    src.request_stop();
    record<int, bool> r2;
    run(*s, receiver_for(r2, stoppable_env(std::get_stop_token, src.get_token())));
    CHECK(r2.how == done::value && std::get<0>(*r2.values) == true);
    s.reset();
    std::this_thread::sync_wait(scope.join());
  }
  // copies associate anew: after close(), a copy is unassociated and completes with stopped,
  // while the original (associated before) still runs
  {
    ex::simple_counting_scope scope;
    auto s = ex::associate(ex::just(7), scope.get_token());
    scope.close();
    auto copy = s; // try_associate fails: closed
    record<int, int> rc;
    run(std::move(copy), receiver_for(rc));
    CHECK(rc.how == done::stopped);
    record<int, int> ro;
    run(std::move(s), receiver_for(ro));
    CHECK(ro.how == done::value && std::get<0>(*ro.values) == 7);
    std::this_thread::sync_wait(scope.join());
  }
  // an associated sender destroyed unstarted releases its association: join can complete
  {
    ex::simple_counting_scope scope;
    {
      auto s = ex::associate(ex::just(), scope.get_token());
      (void)s;
    }
    std::this_thread::sync_wait(scope.join()); // would block forever if the count were 1
  }
}
