// Deregistration scenarios shared by the stop_callback and inplace_stop_callback tests:
// [stoptoken.concepts]/3.2-3.3 and /12. Source is the stoppable-source type, CB<F> the callback
// template. Everything here is deterministic: the only cross-thread scenario uses explicit
// hand-offs, and watchdog() guards against the forbidden blocking.
#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include "check.hpp"

namespace scb {

using Fn = std::function<void()>;

template<class Source, template<class> class CB>
void self_destroy() {
  // (3.3.4) "If callback_fn is executing on the current thread, then the destructor shall not
  // block waiting for the return from the invocation of callback_fn."
  Source src;
  int ran = 0;
  CB<Fn>* self = nullptr;
  self = new CB<Fn>(src.get_token(), Fn([&ran, &self] {
    ++ran;
    CB<Fn>* p = self;
    self = nullptr;
    delete p;  // destroys the callback (and this closure); nothing is touched afterwards
  }));
  CHECK(src.request_stop());
  CHECK(ran == 1 && self == nullptr);
}

template<class Source, template<class> class CB>
void destroy_other() {
  // (3.3.2) a deregistered callback invocation is removed from the stop state, so it is never
  // evaluated; (3.3.5) deregistration does not block on another callback's invocation (the one
  // executing it here). Whichever callback runs first destroys the other: exactly one runs.
  Source src;
  int ran_a = 0, ran_b = 0;
  std::optional<CB<Fn>> a, b;
  a.emplace(src.get_token(), Fn([&] { ++ran_a; if (b) b.reset(); }));
  b.emplace(src.get_token(), Fn([&] { ++ran_b; if (a) a.reset(); }));
  CHECK(src.request_stop());
  CHECK(ran_a + ran_b == 1);
  CHECK(a.has_value() != b.has_value());
}

template<class Source, template<class> class CB>
void nested_registration_and_request() {
  // /12: the stop request is made (atomically) before the callbacks are executed, so inside a
  // callback stop_requested() is true and a second request_stop() returns false; (3.2.1.3.2) a
  // callback constructed then is "immediately evaluated on the thread executing scb's
  // constructor".
  Source src;
  auto tok = src.get_token();
  bool inner_ran = false, saw_requested = false, second = true;
  std::thread::id outer_id, inner_id;
  std::optional<CB<Fn>> inner;
  CB<Fn> outer(tok, Fn([&] {
    outer_id = std::this_thread::get_id();
    saw_requested = tok.stop_requested() && src.stop_requested();
    second = src.request_stop();
    inner.emplace(tok, Fn([&] { inner_ran = true; inner_id = std::this_thread::get_id(); }));
    CHECK(inner_ran);  // evaluated inside the constructor
  }));
  std::thread t([&] { CHECK(src.request_stop()); });
  t.join();
  CHECK(saw_requested && !second && inner_ran);
  CHECK(inner_id == outer_id && outer_id != std::this_thread::get_id());
}

template<class Source, template<class> class CB>
void cross_thread_no_block_on_other() {
  // (3.3.5) "A stoppable callback deregistration shall not block on the completion of the
  // invocation of some other callback registered with the same logical stop state." Callback A
  // runs on the requesting thread and waits until this thread has destroyed callback B.
  Source src;
  std::atomic<int> a_state(0);  // 1: A running, 2: B destroyed
  std::atomic<int> ran_b(0);
  CB<Fn> a(src.get_token(), Fn([&] {
    a_state.store(1);
    a_state.notify_all();
    a_state.wait(1);
  }));
  auto b = std::make_unique<CB<Fn>>(src.get_token(), Fn([&] { ran_b.fetch_add(1); }));
  std::thread t([&] { src.request_stop(); });
  a_state.wait(0);  // A is executing on t
  int before = ran_b.load();  // B ran already (before A) or never will
  b.reset();                  // must not wait for A
  a_state.store(2);
  a_state.notify_all();
  t.join();
  CHECK(ran_b.load() == before);
  CHECK(before == 0 || before == 1);
}

template<class Source, template<class> class CB>
void run_all() {
  self_destroy<Source, CB>();
  destroy_other<Source, CB>();
  nested_registration_and_request<Source, CB>();
  for (int i = 0; i < 20; ++i) cross_thread_no_block_on_other<Source, CB>();
}

}  // namespace scb
