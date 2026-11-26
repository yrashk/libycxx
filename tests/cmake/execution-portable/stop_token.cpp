#include <version>
#if !defined(YCXX_TEST_PORTABLE) || defined(__cpp_lib_jthread)
#include <stop_token>
#endif

#if defined(YCXX_TEST_PORTABLE)
#include <ycxx/execution/stop_token.hpp>
namespace execution = ycxx::execution;
#else
namespace execution = std;
#endif

#include <atomic>
#include <latch>
#include <memory>
#include <optional>
#include <thread>
#include "destroyed_storage.hpp"
#include "stop_callback_scenarios.hpp"

template <class F>
using Callback = execution::inplace_stop_callback<F>;

static void destroy_source_from_callback() {
  destroyed_storage<execution::inplace_stop_source> source;
  source.emplace();
  std::optional<Callback<scb::Fn>> callback;
  callback.emplace(source->get_token(), scb::Fn([&] {
    // Save the source address before destroying the callable that captured it.
    auto* storage = &source;
    callback.reset();
    storage->destroy();
  }));
  CHECK(source->request_stop());
  CHECK(!source.alive());
}

struct CallbackState {
  std::latch entered{1};
  std::latch release{1};
  std::atomic<bool> returned{false};
};

struct BlockingCallback {
  explicit BlockingCallback(CallbackState& state) : state(state) {}

  ~BlockingCallback() { CHECK(state.returned.load(std::memory_order_relaxed)); }

  void operator()() noexcept {
    state.entered.count_down();
    state.release.wait();
    state.returned.store(true, std::memory_order_relaxed);
  }

  CallbackState& state;
};

static void destroy_running_callback() {
  execution::inplace_stop_source source;
  CallbackState state;
  auto callback = std::make_unique<Callback<BlockingCallback>>(source.get_token(), state);
  std::thread requester([&] { CHECK(source.request_stop()); });
  state.entered.wait();

  std::latch deregistering{1};
  std::thread deregistrar([&] {
    deregistering.count_down();
    callback.reset();
  });
  deregistering.wait();
  state.release.count_down();

  deregistrar.join();
  requester.join();
}

int main() {
  scb::run_all<execution::inplace_stop_source, Callback>();
  destroy_source_from_callback();

  for (int i = 0; i != 64; ++i)
    destroy_running_callback();

#if !defined(YCXX_TEST_PORTABLE) || defined(__cpp_lib_jthread)
  // Check coexistence when the host also provides the C++20 stop facilities.
  std::stop_source standard;
  CHECK(standard.request_stop());
#endif
}
