// [exec.when.all]/15-16: a stop requested of the receiver's token runs when_all's on-stop
// callback, stop_src.request_stop(), which runs the children's callbacks. A child that completes
// from inside its own callback, the last to complete, completes when_all from there (on_stop
// reset, then set_stopped) -- and a receiver that destroys the operation state in its completion
// (spawn, let_value's variant, a task frame, a test's storage) destroys stop_src while its
// request_stop() is still running. request_stop must tolerate that ([stopsource.inplace.mem]).
// The destroyed state's storage is overwritten (destroyed_storage.hpp): a request_stop that went
// on would spin on the lock it reads as taken, which the watchdog turns into a failure.
#include <execution>
#include <optional>
#include <stop_token>
#include <utility>
#include "check.hpp"
#include "destroyed_storage.hpp"
#include "exec_support.hpp"
#include "watchdog.hpp"

namespace ex = std::execution;
using namespace exec_test;

// A child that completes with set_stopped from inside its stop callback; Reset: whether it
// deregisters the callback first (else the operation state's destruction deregisters it).
template <bool Reset>
struct stop_completer {
  using sender_concept = ex::sender_tag;
  int* fired;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(), ex::set_stopped_t()>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    using token_t = std::stop_token_of_t<ex::env_of_t<R>>;
    struct on_stop {
      op* o;
      void operator()() noexcept {
        ++*o->fired;
        if constexpr (Reset)
          o->cb.reset();
        ex::set_stopped(std::move(o->r));
      }
    };
    R r;
    int* fired;
    std::optional<std::stop_callback_for_t<token_t, on_stop>> cb;
    void start() & noexcept { cb.emplace(std::get_stop_token(ex::get_env(r)), on_stop{this}); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), fired, std::nullopt};
  }
};

// A child that completes with set_value() at start and leaves its (then pointless) stop callback
// registered: still on stop_src's list when the stop request comes.
struct sloppy_just {
  using sender_concept = ex::sender_tag;
  int* fired;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    using token_t = std::stop_token_of_t<ex::env_of_t<R>>;
    struct on_stop {
      int* fired;
      void operator()() noexcept { ++*fired; }
    };
    R r;
    int* fired;
    std::optional<std::stop_callback_for_t<token_t, on_stop>> cb;
    void start() & noexcept {
      cb.emplace(std::get_stop_token(ex::get_env(r)), on_stop{fired});
      ex::set_value(std::move(r));
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), fired, std::nullopt};
  }
};

// A receiver that records its completion and destroys the operation state from it.
struct completion {
  done how = done::none;
  void (*destroy)(void*) = nullptr;
  void* storage = nullptr;
};
struct destroying_receiver {
  using receiver_concept = ex::receiver_tag;
  completion* c;
  std::inplace_stop_token tok;
  template <class... As>
  void set_value(As&&...) && noexcept {
    c->how = done::value;
    c->destroy(c->storage);
  }
  template <class E>
  void set_error(E&&) && noexcept {
    c->how = done::error;
    c->destroy(c->storage);
  }
  void set_stopped() && noexcept {
    c->how = done::stopped;
    c->destroy(c->storage);
  }
  auto get_env() const noexcept { return ex::env{ex::prop(std::get_stop_token, tok)}; }
};

// Connects sndr to a destroying_receiver on src's token in overwritten storage, starts it and
// requests the stop; returns how it completed.
template <class Sndr>
done request_stop_of(Sndr sndr) {
  std::inplace_stop_source src;
  completion c;
  using op_t = decltype(ex::connect(std::move(sndr), destroying_receiver{&c, src.get_token()}));
  destroyed_storage<op_t> storage;
  c.destroy = [](void* p) { static_cast<destroyed_storage<op_t>*>(p)->destroy(); };
  c.storage = &storage;
  ex::start(storage.emplace_with([&] { return ex::connect(std::move(sndr), destroying_receiver{&c, src.get_token()}); }));
  CHECK(c.how == done::none && storage.alive());
  CHECK(src.request_stop());
  CHECK(!storage.alive());
  return c.how;
}

int main() {
  watchdog(10);
  // The one child's callback completes when_all; the child deregistered it first.
  {
    int fired = 0;
    CHECK(request_stop_of(ex::when_all(stop_completer<true>{&fired}, ex::just())) == done::stopped);
    CHECK(fired == 1);
  }
  // ... and left it registered: the destruction of the operation state deregisters it, from
  // inside its own invocation.
  {
    int fired = 0;
    CHECK(request_stop_of(ex::when_all(stop_completer<false>{&fired}, ex::just())) == done::stopped);
    CHECK(fired == 1);
  }
  // Two such children: the second callback run completes when_all.
  {
    int a = 0, b = 0;
    CHECK(request_stop_of(ex::when_all(stop_completer<true>{&a}, stop_completer<false>{&b})) == done::stopped);
    CHECK(a == 1 && b == 1);
  }
  // The callback that completes when_all is not the last on stop_src's list: the sloppy child's
  // callback, registered before it, is still registered, and is deregistered by the destruction
  // of the operation state. It never runs.
  {
    int sloppy = 0, fired = 0;
    CHECK(request_stop_of(ex::when_all(sloppy_just{&sloppy}, stop_completer<true>{&fired})) == done::stopped);
    CHECK(sloppy == 0 && fired == 1);
  }
  // The scenario as a program meets it: spawn frees the state when when_all completes.
  {
    int fired = 0;
    std::inplace_stop_source src;
    ex::simple_counting_scope scope;
    ex::spawn(ex::when_all(stop_completer<false>{&fired}, ex::just()), scope.get_token(), ex::env{ex::prop(std::get_stop_token, src.get_token())});
    CHECK(fired == 0);
    src.request_stop();
    CHECK(fired == 1);
    scope.close();
    std::this_thread::sync_wait(scope.join());
  }
  return 0;
}
