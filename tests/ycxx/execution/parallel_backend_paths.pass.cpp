// parallel_scheduler over a program's backend ([exec.parschedrepl.query]/3: replaceable):
//   [exec.par.scheduler]/4: two parallel_schedulers are equal iff their backends are the same
//     object (this program's query returns a new backend each time);
//   /7: schedule(sch) calls b.schedule(r, s); the proxy's set_value, set_error(eptr) and
//     set_stopped complete the receiver with the same (/5);
//   /10-12: a bulk_chunked / bulk_unchunked whose child completes on sch calls
//     b.schedule_bulk_chunked / _unchunked with n = shape for parallel_policy and
//     parallel_unsequenced_policy, and n = 1 for the other policies; r.execute(i, j) then runs
//     f(i, j, args...) or f(0, shape, args...) (chunked), f(i, args...) or every f(k, args...)
//     (unchunked), with lvalues of the child's values; the proxy's set_error / set_stopped are
//     forwarded;
//   [exec.parschedrepl.recvproxy]/3-4: try_query<inplace_stop_token>(get_stop_token) returns the
//     receiver's inplace_stop_token.
// REQUIRES: exceptions
#include <execution>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <span>
#include <stop_token>
#include <vector>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
namespace psr = ex::parallel_scheduler_replacement;
using namespace exec_test;

// How the backend completes: 0 value, 1 error(7), 2 stopped.
int mode = 0;
std::size_t last_n = 0;
std::optional<std::inplace_stop_token> seen_token;

template <class Proxy>
void finish(Proxy& r) {
  if (mode == 0)
    r.set_value();
  else if (mode == 1)
    r.set_error(std::make_exception_ptr(7));
  else
    r.set_stopped();
}

struct test_backend : psr::parallel_scheduler_backend {
  void schedule(psr::receiver_proxy& r, std::span<std::byte>) noexcept override {
    seen_token = r.try_query<std::inplace_stop_token>(std::get_stop_token);
    finish(r);
  }
  void schedule_bulk_chunked(std::size_t n, psr::bulk_item_receiver_proxy& r, std::span<std::byte>) noexcept override {
    last_n = n;
    if (mode == 0) {
      // Two chunks when there is room for them.
      if (n > 1) {
        r.execute(0, n / 2);
        r.execute(n / 2, n);
      } else if (n == 1) {
        r.execute(0, 1);
      }
    }
    finish(r);
  }
  void schedule_bulk_unchunked(std::size_t n, psr::bulk_item_receiver_proxy& r, std::span<std::byte>) noexcept override {
    last_n = n;
    if (mode == 0)
      for (std::size_t i = 0; i < n; ++i)
        r.execute(i, i + 1);
    finish(r);
  }
};

namespace std::execution::parallel_scheduler_replacement {
shared_ptr<parallel_scheduler_backend> query_parallel_scheduler_backend() { return make_shared<test_backend>(); }
} // namespace std::execution::parallel_scheduler_replacement

bool holds_7(const std::exception_ptr& e) { return throws_value([&] { std::rethrow_exception(e); }, 7); }

template <class Policy>
void check_chunked(ex::parallel_scheduler sch, Policy pol, bool parallel) {
  std::vector<int> hits(10, 0);
  std::vector<std::pair<int, int>> calls;
  record<std::exception_ptr, int> rec;
  mode = 0;
  run(ex::schedule(sch) | ex::then([] { return 3; }) | ex::bulk_chunked(pol, 10, [&](int b, int e, int& v) {
        calls.emplace_back(b, e);
        for (int i = b; i < e; ++i)
          hits[i] += v;
      }),
      receiver_for(rec));
  CHECK(rec.how == done::value && std::get<0>(*rec.values) == 3);
  for (int h : hits)
    CHECK(h == 3);
  if (parallel) {
    CHECK(last_n == 10 && calls.size() == 2);
  } else {
    CHECK(last_n == 1 && calls.size() == 1 && calls[0] == std::pair(0, 10));
  }
}

template <class Policy>
void check_unchunked(ex::parallel_scheduler sch, Policy pol, bool parallel) {
  std::vector<int> hits(6, 0);
  record<std::exception_ptr, int> rec;
  mode = 0;
  run(ex::schedule(sch) | ex::then([] { return 2; }) | ex::bulk_unchunked(pol, 6, [&](int i, int& v) { hits[i] += v; }),
      receiver_for(rec));
  CHECK(rec.how == done::value && std::get<0>(*rec.values) == 2);
  for (int h : hits)
    CHECK(h == 2);
  CHECK(last_n == (parallel ? 6u : 1u));
}

int main() {
  auto sch = ex::get_parallel_scheduler();
  auto sch2 = ex::get_parallel_scheduler();
  auto copy = sch;
  CHECK(sch == copy && !(sch == sch2) && sch != sch2);

  // schedule
  {
    std::inplace_stop_source src;
    using Env = ex::env<ex::prop<std::get_stop_token_t, std::inplace_stop_token>>;
    for (mode = 0; mode < 3; ++mode) {
      seen_token.reset();
      record<std::exception_ptr> rec;
      run(ex::schedule(sch), receiver_for(rec, Env{ex::prop(std::get_stop_token, src.get_token())}));
      CHECK(rec.calls == 1);
      CHECK(seen_token && *seen_token == src.get_token());
      if (mode == 0)
        CHECK(rec.how == done::value);
      else if (mode == 1)
        CHECK(rec.how == done::error && holds_7(*rec.error));
      else
        CHECK(rec.how == done::stopped);
    }
  }
  // bulk, by policy
  check_chunked(sch, ex::par, true);
  check_chunked(sch, ex::par_unseq, true);
  check_chunked(sch, ex::seq, false);
  check_chunked(sch, ex::unseq, false);
  check_unchunked(sch, ex::par, true);
  check_unchunked(sch, ex::par_unseq, true);
  check_unchunked(sch, ex::seq, false);
  check_unchunked(sch, ex::unseq, false);
  // bulk's error and stopped from the backend are forwarded.
  for (mode = 1; mode < 3; ++mode) {
    int calls = 0;
    const int m = mode;
    record<std::exception_ptr> r1, r2;
    run(ex::schedule(sch) | ex::bulk_chunked(ex::par, 4, [&](int, int) { ++calls; }), receiver_for(r1));
    mode = m;
    run(ex::schedule(sch) | ex::bulk_unchunked(ex::par, 4, [&](int) { ++calls; }), receiver_for(r2));
    mode = m;
    CHECK(calls == 0 && r1.calls == 1 && r2.calls == 1);
    if (mode == 1)
      CHECK(r1.how == done::error && holds_7(*r1.error) && r2.how == done::error && holds_7(*r2.error));
    else
      CHECK(r1.how == done::stopped && r2.how == done::stopped);
  }
  return 0;
}
