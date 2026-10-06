// when_all, when_all_with_variant ([exec.when.all]):
//   when_all(sndrs...) completes once all children have: with the concatenated values (decayed)
//   when all succeed; with the first error otherwise, after requesting a stop of the others;
//   with stopped when one stopped and none failed. The children see a stop token of their own
//   (an inplace_stop_token) that a stop request on the receiver's token also triggers.
//   Each child may have at most one value completion signature. when_all() is ill-formed.
//   when_all_with_variant(sndrs...) is when_all(into_variant(sndrs)...).
// FLAGS: -pthread
// REQUIRES: exceptions
#include <execution>
#include <exception>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>
#include <type_traits>
#include <variant>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;
using namespace exec_test;

// A sender that completes with stopped if a stop is requested of its environment's token when
// it starts, else with its value; records the token's stop state.
struct probe_sender {
  using sender_concept = ex::sender_tag;
  bool* stop_seen;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_stopped_t()>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    bool* stop_seen;
    void start() & noexcept {
      auto tok = std::get_stop_token(ex::get_env(r));
      *stop_seen = tok.stop_requested();
      if (tok.stop_requested())
        ex::set_stopped(std::move(r));
      else
        ex::set_value(std::move(r), 1);
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), stop_seen};
  }
};

int main() {
  // Values concatenated, decayed.
  {
    std::string s = "x";
    auto w = ex::when_all(ex::just(1), ex::just(), ex::just(s, 2.5));
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(w)>,
                                 ex::completion_signatures<ex::set_value_t(int, std::string, double)>>);
    auto r = tt::sync_wait(std::move(w));
    static_assert(std::is_same_v<decltype(r), std::optional<std::tuple<int, std::string, double>>>);
    CHECK(r && std::get<0>(*r) == 1 && std::get<1>(*r) == "x" && std::get<2>(*r) == 2.5);
  }
  // The first error wins; the later children see a stop request.
  {
    bool stop_seen = false;
    record<int, int, int> rec;
    run(ex::when_all(ex::just_error(5), probe_sender{&stop_seen}), receiver_for(rec));
    CHECK(rec.how == done::error && *rec.error == 5);
    CHECK(stop_seen);
    // An exception thrown on the way is the error, rethrown by sync_wait.
    CHECK(throws_value([] { (void)tt::sync_wait(ex::when_all(ex::just(1), ex::just(2) | ex::then([](int x) -> int { throw x + 7; }))); }, 9));
  }
  // Stopped: when a child stops and none fails.
  {
    record<int, int> rec;
    run(ex::when_all(ex::just(1), ex::just_stopped()), receiver_for(rec));
    CHECK(rec.how == done::stopped);
    // just_stopped has no value completion, so neither has the when_all.
    static_assert(!ex::sends_stopped<decltype(ex::just(1))>);
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(ex::when_all(ex::just(1), ex::just_stopped()))>,
                                 ex::completion_signatures<ex::set_stopped_t()>>);
  }
  // A stop requested of the receiver's token reaches the children.
  {
    std::inplace_stop_source src;
    src.request_stop();
    bool stop_seen = false;
    record<int, int, int> rec;
    run(ex::when_all(ex::just(1), probe_sender{&stop_seen}), receiver_for(rec, ex::env{ex::prop(std::get_stop_token, src.get_token())}));
    CHECK(stop_seen);
    CHECK(rec.how == done::stopped);
  }
  // Each child sees an inplace_stop_token.
  {
    auto w = ex::when_all(ex::read_env(std::get_stop_token));
    auto r = tt::sync_wait(std::move(w));
    static_assert(std::is_same_v<decltype(r), std::optional<std::tuple<std::inplace_stop_token>>>);
    CHECK(r && std::get<0>(*r).stop_possible());
  }
  static_assert(!std::is_invocable_v<ex::when_all_t>);
  static_assert(!std::is_invocable_v<ex::when_all_t, int>);

  // when_all_with_variant
  {
    auto w = ex::when_all_with_variant(ex::just(1), ex::just(std::string("a")));
    auto r = tt::sync_wait(std::move(w));
    static_assert(std::is_same_v<decltype(r), std::optional<std::tuple<std::variant<std::tuple<int>>, std::variant<std::tuple<std::string>>>>>);
    CHECK(r && std::get<0>(std::get<0>(std::get<0>(*r))) == 1);
    CHECK(std::get<0>(std::get<0>(std::get<1>(*r))) == "a");
  }
  // Children completing on other threads.
  {
    ex::run_loop l1, l2;
    std::thread t1([&] { l1.run(); }), t2([&] { l2.run(); });
    auto w = ex::when_all(ex::schedule(l1.get_scheduler()) | ex::then([] { return tt::get_id(); }),
                          ex::schedule(l2.get_scheduler()) | ex::then([] { return tt::get_id(); }));
    auto r = tt::sync_wait(std::move(w));
    const auto id1 = t1.get_id(), id2 = t2.get_id();
    l1.finish();
    l2.finish();
    t1.join();
    t2.join();
    CHECK(r && std::get<0>(*r) == id1 && std::get<1>(*r) == id2);
  }
  return 0;
}
