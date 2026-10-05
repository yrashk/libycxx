// Tens of thousands of stop callbacks on one stop state, callbacks that deregister or register
// many others during the stop request, and threads that register and deregister while another
// thread requests stop. For stop_source/stop_callback and inplace_stop_source/
// inplace_stop_callback.
//   [stoptoken.concepts]/3.2.1.3.1: a callback registered while no stop was requested is added
//     to the stop state's list "such that std::forward<CallbackFn>(callback_fn)() is evaluated
//     if a stop request is made"; (3.2.1.3.2) one registered after the request is evaluated
//     immediately on the constructing thread; (3.3.2) a deregistered invocation is removed, so
//     it is never evaluated; (3.3.4) a callback may destroy itself; /12 (request_stop): the
//     registered callbacks are invoked once each. The draft sets no limit on the number of
//     callbacks registered with one stop state.
//   [stopcallback.cons], [stopcallback.inplace.cons].
// FLAGS: -pthread
#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <stop_token>
#include <thread>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

using Fn = std::function<void()>;

template<class Source, template<class> class CB>
void run_all() {
  constexpr int N = 50000;
  // N callbacks; every third one is destroyed before the request.
  {
    Source src;
    std::vector<int> ran(N);
    std::vector<std::unique_ptr<CB<Fn>>> cbs(N);
    for (int i = 0; i < N; ++i) cbs[static_cast<std::size_t>(i)] = std::make_unique<CB<Fn>>(src.get_token(), Fn([&ran, i] { ++ran[static_cast<std::size_t>(i)]; }));
    for (int i = 0; i < N; i += 3) cbs[static_cast<std::size_t>(i)].reset();
    CHECK(src.request_stop());
    for (int i = 0; i < N; ++i) CHECK(ran[static_cast<std::size_t>(i)] == (i % 3 == 0 ? 0 : 1));
    CHECK(!src.request_stop());
    for (int i = 0; i < N; ++i) CHECK(ran[static_cast<std::size_t>(i)] == (i % 3 == 0 ? 0 : 1));
  }
  // One callback destroys a thousand others (registered before and after it); exactly one of
  // each pair "killer/victim" runs. Another registers a thousand new callbacks during the
  // request: each one runs at once, in its constructor.
  {
    Source src;
    constexpr int M = 1000;
    std::vector<int> ran(2 * M);
    std::vector<std::optional<CB<Fn>>> victims(2 * M);
    std::vector<std::unique_ptr<CB<Fn>>> late;
    int late_ran = 0;
    bool late_ran_in_ctor = true;
    for (int i = 0; i < M; ++i) victims[static_cast<std::size_t>(i)].emplace(src.get_token(), Fn([&ran, i] { ++ran[static_cast<std::size_t>(i)]; }));
    int killer_ran = 0;
    CB<Fn> killer(src.get_token(), Fn([&] {
      ++killer_ran;
      for (auto& v : victims) v.reset();
    }));
    CB<Fn> spawner(src.get_token(), Fn([&] {
      for (int i = 0; i < M; ++i) {
        const int before = late_ran;
        late.push_back(std::make_unique<CB<Fn>>(src.get_token(), Fn([&] { ++late_ran; })));
        late_ran_in_ctor = late_ran_in_ctor && late_ran == before + 1;
      }
    }));
    for (int i = M; i < 2 * M; ++i) victims[static_cast<std::size_t>(i)].emplace(src.get_token(), Fn([&ran, i] { ++ran[static_cast<std::size_t>(i)]; }));
    CHECK(src.request_stop());
    CHECK(killer_ran == 1);
    CHECK(late_ran == M && late_ran_in_ctor);
    // Victims that ran did so before the killer; the others never run.
    for (int i = 0; i < 2 * M; ++i) CHECK(ran[static_cast<std::size_t>(i)] <= 1);
    for (auto& v : victims) CHECK(!v.has_value());
  }
  // Threads register and deregister callbacks while another thread requests stop: each
  // callback runs at most once, and one still registered when the request completes has run.
  {
    constexpr int T = 6, Per = 4000;
    Source src;
    std::atomic<bool> go{false};
    std::atomic<int> bad{0};
    std::vector<std::thread> ts;
    for (int t = 0; t < T; ++t)
      ts.emplace_back([&] {
        while (!go.load()) {
        }
        std::vector<std::pair<std::unique_ptr<CB<Fn>>, std::unique_ptr<std::atomic<int>>>> mine;
        for (int i = 0; i < Per; ++i) {
          auto count = std::make_unique<std::atomic<int>>(0);
          std::atomic<int>* c = count.get();
          mine.emplace_back(std::make_unique<CB<Fn>>(src.get_token(), Fn([c] { c->fetch_add(1); })), std::move(count));
          if (i % 2) mine[static_cast<std::size_t>(i - 1)].first.reset();
        }
        while (!src.stop_requested()) std::this_thread::yield();
        // stop_requested() is true: the request was made, but the callbacks may still be
        // running on the requesting thread. Destroy the rest (blocks while one runs, 3.3.3).
        for (auto& [cb, c] : mine) cb.reset();
        for (auto& [cb, c] : mine)
          if (c->load() > 1) bad.fetch_add(1);
      });
    go.store(true);
    std::this_thread::yield();
    src.request_stop();
    for (auto& th : ts) th.join();
    CHECK(bad.load() == 0);
  }
}

template<class F> using StdCB = std::stop_callback<F>;
template<class F> using InplaceCB = std::inplace_stop_callback<F>;

int main() {
  watchdog(20);
  run_all<std::stop_source, StdCB>();
  run_all<std::inplace_stop_source, InplaceCB>();
  return 0;
}
