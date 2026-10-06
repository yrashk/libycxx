// [exec.when.all]/16: when_all's start registers a stop callback on the receiver's stop token
// that requests a stop of its own inplace_stop_source, whose token its children see (/6.2): a
// stop requested of the receiver's token while the children run reaches them. /17: a child's
// error or stopped completion requests a stop of the others. /15: when_all deregisters that
// callback (on_stop.reset()) before it completes, so a stop requested of the receiver's token
// afterwards no longer reaches the children's token.
#include <execution>
#include <optional>
#include <stop_token>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

// A child that completes when the test says so (finish()): with set_stopped if a stop was
// requested of its token by then, else with set_value(). Records the stop request.
struct waiter_state {
  bool started = false;
  bool stop_seen = false;
  void (*finish_fn)(void*) = nullptr;
  void* op = nullptr;
  std::optional<std::inplace_stop_token> token; // the token it was given
  void finish() { finish_fn(op); }
};

struct waiter {
  using sender_concept = ex::sender_tag;
  waiter_state* st;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(), ex::set_stopped_t()>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    using token_t = std::stop_token_of_t<ex::env_of_t<R>>;
    struct on_stop {
      waiter_state* st;
      void operator()() noexcept { st->stop_seen = true; }
    };
    R r;
    waiter_state* st;
    std::optional<std::stop_callback_for_t<token_t, on_stop>> cb;
    void start() & noexcept {
      st->started = true;
      auto tok = std::get_stop_token(ex::get_env(r));
      if constexpr (std::is_same_v<token_t, std::inplace_stop_token>)
        st->token = tok;
      st->op = this;
      st->finish_fn = [](void* p) { static_cast<op*>(p)->finish(); };
      cb.emplace(tok, on_stop{st});
    }
    void finish() {
      cb.reset();
      if (st->stop_seen)
        ex::set_stopped(std::move(r));
      else
        ex::set_value(std::move(r));
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), st, std::nullopt};
  }
};

int main() {
  using Env = ex::env<ex::prop<std::get_stop_token_t, std::inplace_stop_token>>;
  // A stop requested of the receiver's token reaches every child.
  {
    std::inplace_stop_source src;
    waiter_state a, b;
    record<int> rec;
    auto op = ex::connect(ex::when_all(waiter{&a}, waiter{&b}), receiver_for(rec, Env{ex::prop(std::get_stop_token, src.get_token())}));
    ex::start(op);
    CHECK(a.started && b.started && !a.stop_seen && !b.stop_seen && rec.how == done::none);
    src.request_stop();
    CHECK(a.stop_seen && b.stop_seen && rec.how == done::none);
    a.finish();
    CHECK(rec.how == done::none);
    b.finish();
    CHECK(rec.how == done::stopped && rec.calls == 1);
  }
  // A child's stopped completion requests a stop of the others.
  {
    waiter_state a;
    record<int> rec;
    auto op = ex::connect(ex::when_all(waiter{&a}, ex::just_stopped()), receiver_for(rec));
    ex::start(op);
    CHECK(a.stop_seen && rec.how == done::none);
    a.finish();
    CHECK(rec.how == done::stopped && rec.calls == 1);
  }
  // A child's error requests a stop of the others; the result is the error.
  {
    waiter_state a;
    record<int> rec;
    auto op = ex::connect(ex::when_all(waiter{&a}, ex::just_error(5)), receiver_for(rec));
    ex::start(op);
    CHECK(a.stop_seen && rec.how == done::none);
    a.finish();
    CHECK(rec.how == done::error && *rec.error == 5);
  }
  // After when_all completed, a stop requested of the receiver's token no longer reaches the
  // children's token (the operation state still exists).
  {
    std::inplace_stop_source src;
    waiter_state a;
    record<int> rec;
    auto op = ex::connect(ex::when_all(waiter{&a}), receiver_for(rec, Env{ex::prop(std::get_stop_token, src.get_token())}));
    ex::start(op);
    CHECK(a.token && a.token->stop_possible() && !a.token->stop_requested());
    a.finish();
    CHECK(rec.how == done::value);
    src.request_stop();
    CHECK(!a.token->stop_requested());
  }
  return 0;
}
