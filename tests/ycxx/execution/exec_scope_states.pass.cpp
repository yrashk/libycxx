// The states of simple_counting_scope and counting_scope ([exec.counting.scopes.general]/1) and
// what each allows:
//   [exec.simple.counting.mem]/2-3: close() on an unused scope makes it unused-and-closed; after
//     close() every try-associate fails; /5-6: try-associate succeeds while unused, open or
//     open-and-joining (the count goes up), and returns a disengaged association otherwise;
//   [exec.simple.counting.mem]/9 and [exec.counting.scopes.general]/4: a join started while the
//     count is 0 completes inline (joined); otherwise it is registered (open-and-joining, or
//     closed-and-joining when closed) and completes, through schedule(get_start_scheduler(env)),
//     when the last association ends (/8);
//   [exec.counting.scopes.general]/5.2: an association is engaged iff it refers to a scope, and
//     releasing it (destroying it) disassociates;
//   [exec.spawn]/7: spawn starts the sender only if the association succeeded; otherwise the
//     sender never runs;
//   [exec.simple.counting.ctor]/2: destroying a scope in state unused, unused-and-closed or
//     joined has no effects (the destructors below must not terminate).
// FLAGS: -pthread
#include <execution>
#include <optional>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

using join_env = ex::prop<ex::get_start_scheduler_t, ex::inline_scheduler>;

template <class Scope>
void unused_and_closed() {
  Scope scope;
  scope.close();
  auto a = scope.get_token().try_associate();
  CHECK(!a);
  bool ran = false;
  ex::spawn(ex::just() | ex::then([&]() noexcept { ran = true; }), scope.get_token());
  CHECK(!ran); // the association failed: the sender is not started
} // unused-and-closed: no effects

template <class Scope>
void join_unused_inline() {
  Scope scope;
  record<int> r;
  auto op = ex::connect(scope.join(), receiver_for(r, join_env(ex::get_start_scheduler, ex::inline_scheduler{})));
  ex::start(op);
  CHECK(r.how == done::value && r.calls == 1); // count 0: completes inline
  // joined: new associations fail
  CHECK(!scope.get_token().try_associate());
}

template <class Scope>
void join_waits_for_associations() {
  Scope scope;
  auto tok = scope.get_token();
  std::optional<decltype(tok.try_associate())> a1, a2;
  a1.emplace(tok.try_associate());
  CHECK(bool(*a1));
  record<int> r;
  auto op = ex::connect(scope.join(), receiver_for(r, join_env(ex::get_start_scheduler, ex::inline_scheduler{})));
  ex::start(op);
  CHECK(r.how == done::none); // open-and-joining
  a2.emplace(tok.try_associate());
  CHECK(bool(*a2)); // still possible while open-and-joining
  scope.close();    // closed-and-joining
  CHECK(!tok.try_associate());
  a1.reset();
  CHECK(r.how == done::none); // one association left
  a2.reset();
  CHECK(r.how == done::value && r.calls == 1); // joined
}

template <class Scope>
void used_closed_then_joined() {
  Scope scope;
  int runs = 0;
  ex::spawn(ex::just() | ex::then([&]() noexcept { ++runs; }), scope.get_token());
  scope.close(); // open -> closed
  ex::spawn(ex::just() | ex::then([&]() noexcept { ++runs; }), scope.get_token());
  CHECK(runs == 1);
  std::this_thread::sync_wait(scope.join()); // count 0: joined
}

int main() {
  unused_and_closed<ex::simple_counting_scope>();
  unused_and_closed<ex::counting_scope>();
  join_unused_inline<ex::simple_counting_scope>();
  join_unused_inline<ex::counting_scope>();
  join_waits_for_associations<ex::simple_counting_scope>();
  join_waits_for_associations<ex::counting_scope>();
  used_closed_then_joined<ex::simple_counting_scope>();
  used_closed_then_joined<ex::counting_scope>();
}
