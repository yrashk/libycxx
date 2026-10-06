// Execution scopes ([exec.scope]) and the algorithms on scope tokens:
//   [exec.scope.concepts]: the tokens of both counting scopes model scope_token; their
//     associations model scope_association (engaged iff associated; moving disengages the source).
//   [exec.counting.scopes]: try_associate succeeds while open (or unused); close() makes later
//     associations fail; join() completes once the count of associations is zero, on the
//     receiver's start scheduler; a scope may be destroyed when joined or never used.
//   [exec.scope.counting]: counting_scope's token wraps senders so that request_stop() stops them.
//   [exec.spawn]: spawn(sndr, token) starts sndr eagerly in the scope (only if it can associate).
//   [exec.spawn.future]: spawn_future returns a sender of sndr's result; destroying it unstarted
//     requests a stop of the eagerly started work; a stop request racing the work's completion
//     completes the future exactly once.
//   [exec.associate]: associate(sndr, token) completes as sndr, or with set_stopped if the
//     association failed.
// FLAGS: -pthread
#include <execution>
#include <atomic>
#include <thread>
#include <type_traits>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;
using namespace exec_test;

static_assert(ex::scope_token<ex::simple_counting_scope::token>);
static_assert(ex::scope_token<ex::counting_scope::token>);
static_assert(ex::scope_association<decltype(std::declval<ex::simple_counting_scope::token&>().try_associate())>);
static_assert(!ex::scope_token<int>);
static_assert(!std::is_move_constructible_v<ex::simple_counting_scope>);
static_assert(ex::simple_counting_scope::max_associations > 1000);

// A sender that completes only when its receiver's stop token is stopped (or when released).
struct wait_for_stop {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(), ex::set_stopped_t()>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    struct on_stop {
      op* self;
      void operator()() noexcept { ex::set_stopped(std::move(self->r)); }
    };
    using cb_t = std::stop_callback_for_t<std::stop_token_of_t<ex::env_of_t<R>>, on_stop>;
    R r;
    std::optional<cb_t> cb;
    void start() & noexcept { cb.emplace(std::get_stop_token(ex::get_env(r)), on_stop{this}); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), std::nullopt};
  }
};

int main() {
  // Associations.
  {
    ex::simple_counting_scope scope;
    auto tok = scope.get_token();
    auto a = tok.try_associate();
    CHECK(static_cast<bool>(a));
    auto b = a.try_associate();
    CHECK(static_cast<bool>(b));
    auto c = std::move(a);
    CHECK(!static_cast<bool>(a) && static_cast<bool>(c));
    decltype(a) none;
    CHECK(!static_cast<bool>(none));
    CHECK(!static_cast<bool>(none.try_associate()));
    scope.close();
    CHECK(!static_cast<bool>(tok.try_associate()));
    b = decltype(b)();
    c = decltype(c)();
    tt::sync_wait(scope.join());
  }
  // An unused scope may be destroyed; so may an unused closed one.
  {
    ex::simple_counting_scope s1;
    ex::counting_scope s2;
    s2.close();
  }
  // spawn and join.
  {
    ex::simple_counting_scope scope;
    std::atomic<int> ran{0};
    for (int i = 0; i < 5; ++i)
      ex::spawn(ex::just() | ex::then([&]() noexcept { ++ran; }), scope.get_token());
    tt::sync_wait(scope.join());
    CHECK(ran == 5);
    // After join, the scope accepts nothing.
    ex::spawn(ex::just() | ex::then([&]() noexcept { ++ran; }), scope.get_token());
    CHECK(ran == 5);
  }
  // join waits for work running on another thread.
  {
    ex::run_loop loop;
    std::thread t([&] { loop.run(); });
    ex::counting_scope scope;
    std::atomic<int> ran{0};
    for (int i = 0; i < 3; ++i)
      ex::spawn(ex::schedule(loop.get_scheduler()) | ex::then([&]() noexcept { ++ran; }), scope.get_token());
    tt::sync_wait(scope.join());
    CHECK(ran == 3);
    loop.finish();
    t.join();
  }
  // counting_scope::request_stop stops the work of the scope.
  {
    ex::counting_scope scope;
    std::atomic<int> stopped{0};
    for (int i = 0; i < 2; ++i)
      ex::spawn(wait_for_stop{} | ex::upon_stopped([&]() noexcept { ++stopped; }), scope.get_token());
    CHECK(stopped == 0);
    scope.request_stop();
    CHECK(stopped == 2);
    tt::sync_wait(scope.join());
  }
  // spawn_future
  {
    ex::counting_scope scope;
    auto f = ex::spawn_future(ex::just(5, 'x'), scope.get_token());
    auto r = tt::sync_wait(std::move(f));
    CHECK(r && std::get<0>(*r) == 5 && std::get<1>(*r) == 'x');
    // Work that has not completed when the future is dropped is asked to stop.
    bool stopped = false;
    {
      auto g = ex::spawn_future(wait_for_stop{} | ex::upon_stopped([&]() noexcept { stopped = true; }), scope.get_token());
      CHECK(!stopped);
    }
    CHECK(stopped);
    // A future connected and started: its result arrives when the work completes.
    ex::run_loop loop;
    std::thread t([&] { loop.run(); });
    auto h = ex::spawn_future(ex::schedule(loop.get_scheduler()) | ex::then([] { return 7; }), scope.get_token());
    auto r2 = tt::sync_wait(std::move(h));
    CHECK(r2 && std::get<0>(*r2) == 7);
    // Stopping the consumer of a future whose work is still running.
    std::inplace_stop_source src;
    auto k = ex::spawn_future(wait_for_stop{}, scope.get_token());
    record<int> rec;
    auto op = ex::connect(std::move(k), receiver_for(rec, ex::env{ex::prop(std::get_stop_token, src.get_token())}));
    ex::start(op);
    CHECK(rec.how == done::none);
    src.request_stop();
    CHECK(rec.how == done::stopped);
    tt::sync_wait(scope.join());
    loop.finish();
    t.join();
  }
  // spawn_future: a stop request racing the spawned work's completion on another thread; the
  // future completes exactly once, with the value or with set_stopped (TSan/ASan check the state's
  // lifetime).
  {
    ex::counting_scope scope;
    auto sch = ex::get_parallel_scheduler();
    int values = 0, stops = 0;
    for (int i = 0; i < 200; ++i) {
      std::inplace_stop_source src;
      auto f = ex::spawn_future(ex::starts_on(sch, ex::just(i)), scope.get_token());
      std::thread t([&] { src.request_stop(); });
      auto r = tt::sync_wait(ex::write_env(std::move(f), ex::prop(std::get_stop_token, src.get_token())));
      t.join();
      if (r) {
        CHECK(std::get<0>(*r) == i);
        ++values;
      } else {
        ++stops;
      }
    }
    CHECK(values + stops == 200);
    tt::sync_wait(scope.join());
  }
  // associate
  {
    ex::counting_scope scope;
    auto r = tt::sync_wait(ex::just(3) | ex::associate(scope.get_token()));
    CHECK(r && std::get<0>(*r) == 3);
    auto s = ex::associate(ex::just(4), scope.get_token()); // associates now
    scope.close();
    auto r2 = tt::sync_wait(std::move(s));
    CHECK(r2 && std::get<0>(*r2) == 4);
    auto r3 = tt::sync_wait(ex::associate(ex::just(5), scope.get_token())); // closed: stopped
    CHECK(!r3);
    tt::sync_wait(scope.join());
  }
  return 0;
}
