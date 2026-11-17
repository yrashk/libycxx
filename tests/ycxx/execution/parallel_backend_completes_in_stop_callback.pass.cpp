// [exec.parschedrepl.recvproxy]/4-5: a receiver whose stop token is not an inplace_stop_token
// gets, from try_query<inplace_stop_token>(get_stop_token), a token of the proxy's own
// inplace_stop_source, into which a callback on the receiver's token forwards stop requests. A
// backend that registers a callback on that token and completes the proxy from inside it (the
// natural way to cancel queued work) completes the receiver from inside the proxy's
// request_stop(); a receiver that destroys the operation state in its completion destroys the
// proxy's source while its request_stop() still runs. request_stop must tolerate that
// ([stopsource.inplace.mem]).
// The destroyed state's storage is overwritten (destroyed_storage.hpp): a request_stop that went
// on would spin on the lock it reads as taken, which the watchdog turns into a failure.
// REQUIRES: exceptions
#include <execution>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <span>
#include <stop_token>
#include <utility>
#include "check.hpp"
#include "destroyed_storage.hpp"
#include "exec_support.hpp"
#include "watchdog.hpp"

namespace ex = std::execution;
namespace psr = ex::parallel_scheduler_replacement;
using namespace exec_test;

// The backend: schedule registers a callback on the proxy's token that completes the proxy with
// set_stopped (deregistering itself first) and otherwise leaves the work queued (never run here).
struct on_stop {
  psr::receiver_proxy* r;
  void operator()() noexcept;
};
std::optional<std::inplace_stop_callback<on_stop>> queued;
int fired = 0;
void on_stop::operator()() noexcept {
  ++fired;
  psr::receiver_proxy* p = r;
  queued.reset();
  p->set_stopped();
}

struct test_backend : psr::parallel_scheduler_backend {
  void schedule(psr::receiver_proxy& r, std::span<std::byte>) noexcept override {
    auto tok = r.try_query<std::inplace_stop_token>(std::get_stop_token);
    CHECK(tok && tok->stop_possible() && !tok->stop_requested());
    queued.emplace(*tok, on_stop{&r});
  }
  void schedule_bulk_chunked(std::size_t, psr::bulk_item_receiver_proxy& r, std::span<std::byte>) noexcept override { r.set_error(std::make_exception_ptr(0)); }
  void schedule_bulk_unchunked(std::size_t, psr::bulk_item_receiver_proxy& r, std::span<std::byte>) noexcept override { r.set_error(std::make_exception_ptr(0)); }
};

namespace std::execution::parallel_scheduler_replacement {
shared_ptr<parallel_scheduler_backend> query_parallel_scheduler_backend() { return make_shared<test_backend>(); }
} // namespace std::execution::parallel_scheduler_replacement

struct completion {
  done how = done::none;
  void (*destroy)(void*) = nullptr;
  void* storage = nullptr;
};
struct destroying_receiver {
  using receiver_concept = ex::receiver_tag;
  completion* c;
  std::stop_token tok;
  void set_value() && noexcept {
    c->how = done::value;
    c->destroy(c->storage);
  }
  void set_error(std::exception_ptr) && noexcept {
    c->how = done::error;
    c->destroy(c->storage);
  }
  void set_stopped() && noexcept {
    c->how = done::stopped;
    c->destroy(c->storage);
  }
  auto get_env() const noexcept { return ex::env{ex::prop(std::get_stop_token, tok)}; }
};

int main() {
  watchdog(10);
  std::stop_source src;
  completion c;
  auto sch = ex::get_parallel_scheduler();
  using op_t = decltype(ex::connect(ex::schedule(sch), destroying_receiver{&c, src.get_token()}));
  destroyed_storage<op_t> storage;
  c.destroy = [](void* p) { static_cast<destroyed_storage<op_t>*>(p)->destroy(); };
  c.storage = &storage;
  ex::start(storage.emplace_with([&] { return ex::connect(ex::schedule(sch), destroying_receiver{&c, src.get_token()}); }));
  CHECK(c.how == done::none && storage.alive() && queued && fired == 0);
  CHECK(src.request_stop());
  CHECK(c.how == done::stopped && !storage.alive() && !queued && fired == 1);
  return 0;
}
