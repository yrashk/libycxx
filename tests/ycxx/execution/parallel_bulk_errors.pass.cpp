// bulk, bulk_chunked and bulk_unchunked on the parallel scheduler (its domain,
// [exec.par.scheduler]/8-12, with the default backend) when f throws: [exec.bulk]/9.1.2 lets the
// operation run a subset of the invocations, then requires the error completion's exception_ptr
// to hold the exception f threw, a bad_alloc, or an exception derived from runtime_error. An
// error of the child is forwarded without calling f (/9.2, [exec.par.scheduler]/11.2, /12.2).
// The completion signatures have the exception_ptr error ([exec.bulk]/5, /7).
// FLAGS: -pthread
// REQUIRES: exceptions
#include <execution>
#include <atomic>
#include <exception>
#include <new>
#include <stdexcept>
#include <thread>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

struct boom {
  int v;
};

// The error is f's exception or one of the two others /9.1.2 allows.
template <class F>
bool fails_as_allowed(F run, int v) {
  try {
    run();
  } catch (const boom& b) {
    return b.v == v;
  } catch (const std::bad_alloc&) {
    return true;
  } catch (const std::runtime_error&) {
    return true;
  } catch (...) {
  }
  return false;
}

int main() {
  auto sch = ex::get_parallel_scheduler();
  using EP = std::exception_ptr;
  {
    using S = decltype(ex::schedule(sch) | ex::bulk_chunked(ex::par, 100, [](int, int) { throw boom{1}; }));
    static_assert([]<class... Sigs>(ex::completion_signatures<Sigs...>*) {
      return (std::is_same_v<Sigs, ex::set_error_t(EP)> || ...);
    }(static_cast<ex::completion_signatures_of_t<S, ex::env<>>*>(nullptr)));
  }
  CHECK(fails_as_allowed(
      [&] {
        (void)tt::sync_wait(ex::schedule(sch) | ex::bulk_chunked(ex::par, 100, [](int b, int e) {
                              if (b <= 50 && 50 < e)
                                throw boom{1};
                            }));
      },
      1));
  CHECK(fails_as_allowed(
      [&] {
        (void)tt::sync_wait(ex::schedule(sch) | ex::bulk_unchunked(ex::par, 64, [](int i) {
                              if (i % 7 == 3)
                                throw boom{2};
                            }));
      },
      2));
  CHECK(fails_as_allowed(
      [&] {
        (void)tt::sync_wait(ex::schedule(sch) | ex::then([] { return 5; }) | ex::bulk(ex::par_unseq, 40, [](int i, int v) {
                              if (i == 39)
                                throw boom{v};
                            }));
      },
      5));
  // seq: one agent runs every index; the error is still f's (or an allowed one).
  CHECK(fails_as_allowed(
      [&] {
        (void)tt::sync_wait(ex::schedule(sch) | ex::bulk(ex::seq, 10, [](int i) {
                              if (i == 4)
                                throw boom{6};
                            }));
      },
      6));
  // The child's error: forwarded, f not called.
  {
    std::atomic<int> calls{0};
    bool caught = false;
    try {
      (void)tt::sync_wait(ex::schedule(sch) | ex::then([]() -> int { throw boom{7}; }) |
                          ex::bulk_unchunked(ex::par, 8, [&](int, int) { ++calls; }));
    } catch (const boom& b) {
      caught = b.v == 7;
    }
    CHECK(caught && calls == 0);
  }
  // Without exceptions, every index once.
  {
    std::atomic<int> hits[50] = {};
    auto r = tt::sync_wait(ex::schedule(sch) | ex::bulk(ex::par, 50, [&](int i) { ++hits[i]; }));
    CHECK(r.has_value());
    for (auto& h : hits)
      CHECK(h == 1);
  }
  return 0;
}
