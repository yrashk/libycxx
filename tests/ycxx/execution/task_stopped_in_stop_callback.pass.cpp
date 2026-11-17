// [task.state]/5.2: a task whose receiver's stop token is not its stop_token_type owns a stop
// source, which a callback on the receiver's token forwards stop requests into. That forwarding
// request_stop runs the awaited operation's stop callback; an operation that completes with
// set_stopped from inside it completes the task from there ([task.promise]/8: the frame is
// destroyed, set_stopped(rcvr)), and a receiver that destroys the operation state in its
// completion destroys the task's source while its request_stop() is still running. request_stop
// must tolerate that ([stopsource.inplace.mem], [stopsource.mem]).
// The destroyed state's storage is overwritten (destroyed_storage.hpp): a request_stop that went
// on would spin on the lock it reads as taken, which the watchdog turns into a failure.
// REQUIRES: exceptions
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

// A sender that completes with set_stopped from inside its stop callback (left registered: the
// frame's destruction deregisters it from inside the invocation).
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

struct completion {
  done how = done::none;
  void (*destroy)(void*) = nullptr;
  void* storage = nullptr;
};
template <class Token>
struct destroying_receiver {
  using receiver_concept = ex::receiver_tag;
  completion* c;
  Token tok;
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

template <class Source, class Task>
done request_stop_of(Task (*work)(int*), int* fired) {
  Source src;
  completion c;
  using rcvr_t = destroying_receiver<decltype(src.get_token())>;
  using op_t = decltype(ex::connect(work(fired), rcvr_t{&c, src.get_token()}));
  destroyed_storage<op_t> storage;
  c.destroy = [](void* p) { static_cast<destroyed_storage<op_t>*>(p)->destroy(); };
  c.storage = &storage;
  ex::start(storage.emplace_with([&] { return ex::connect(work(fired), rcvr_t{&c, src.get_token()}); }));
  CHECK(c.how == done::none && storage.alive() && *fired == 0);
  CHECK(src.request_stop());
  CHECK(!storage.alive() && *fired == 1);
  return c.how;
}

// The task's inplace_stop_source, forwarded into from a std::stop_token.
struct inplace_env {
  using start_scheduler_type = ex::inline_scheduler;
};
ex::task<void, inplace_env> inplace_work(int* fired) { co_await stop_completer{fired}; }

// The task's stop_source (whose shared state is on the heap), forwarded into from an
// inplace_stop_token.
struct shared_env {
  using start_scheduler_type = ex::inline_scheduler;
  using stop_source_type = std::stop_source;
};
ex::task<void, shared_env> shared_work(int* fired) { co_await stop_completer{fired}; }

int main() {
  watchdog(10);
  int fired = 0;
  CHECK(request_stop_of<std::stop_source>(&inplace_work, &fired) == done::stopped);
  fired = 0;
  CHECK(request_stop_of<std::inplace_stop_source>(&shared_work, &fired) == done::stopped);
  return 0;
}
