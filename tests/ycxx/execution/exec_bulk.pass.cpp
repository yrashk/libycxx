// bulk, bulk_chunked, bulk_unchunked ([exec.bulk]):
//   bulk(sndr, policy, shape, f) calls f(i, args...) for every i in [0, shape) with lvalues of
//   sndr's values, then sends the values; bulk_unchunked likewise; bulk_chunked calls
//   f(b, e, args...) for pairs covering [0, shape) exactly once. Errors and stopped pass through.
//   An exception from f becomes set_error(exception_ptr) ([exec.bulk]/9.1.2).
//   Ill-formed: not a sender, not an execution policy, a non-integral shape, a non-copyable f.
// REQUIRES: exceptions
#include <execution>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

int main() {
  // bulk: every index once, with the values; the values are then sent on.
  {
    std::vector<int> hits(10, 0);
    auto s = ex::just(100) | ex::bulk(ex::seq, 10, [&](int i, int& v) {
               hits[i] += 1;
               CHECK(v == 100);
             });
    record<std::exception_ptr, int> r;
    run(s, receiver_for(r));
    CHECK(r.how == done::value && std::get<0>(*r.values) == 100);
    for (int h : hits)
      CHECK(h == 1);
  }
  // bulk_chunked: the chunks cover [0, shape) exactly once.
  {
    std::vector<int> hits(17, 0);
    auto s = ex::just() | ex::bulk_chunked(ex::par, 17, [&](int b, int e) {
               CHECK(b < e);
               for (int i = b; i < e; ++i)
                 hits[i] += 1;
             });
    record<std::exception_ptr> r;
    run(s, receiver_for(r));
    CHECK(r.how == done::value);
    for (int h : hits)
      CHECK(h == 1);
  }
  // bulk_unchunked
  {
    long sum = 0;
    record<std::exception_ptr, long> r;
    run(ex::just(5L) | ex::bulk_unchunked(ex::seq, 4, [&](int i, long& v) { sum += i * v; }), receiver_for(r));
    CHECK(r.how == done::value && sum == 30 && std::get<0>(*r.values) == 5);
  }
  // shape 0: no calls; the values are sent.
  {
    int calls = 0;
    record<std::exception_ptr, int> r;
    run(ex::just(1) | ex::bulk(ex::seq, 0, [&](int, int) { ++calls; }), receiver_for(r));
    CHECK(r.how == done::value && calls == 0);
  }
  // Errors pass through without calling f; an exception from f is an error.
  {
    int calls = 0;
    record<int> r;
    run(ex::just_error(3) | ex::bulk(ex::seq, 5, [&](int) noexcept { ++calls; }), receiver_for(r));
    CHECK(r.how == done::error && *r.error == 3 && calls == 0);
    record<std::exception_ptr> r2;
    run(ex::just() | ex::bulk(ex::seq, 5, [](int i) {
          if (i == 2)
            throw std::runtime_error("bulk");
        }),
        receiver_for(r2));
    CHECK(r2.how == done::error && r2.error.has_value());
  }
  // The signatures: no exception_ptr error for a nothrow f.
  {
    using S = decltype(ex::just(1) | ex::bulk_unchunked(ex::seq, 3, [](int, int) noexcept {}));
    static_assert(std::is_same_v<ex::completion_signatures_of_t<S>, ex::completion_signatures<ex::set_value_t(int)>>);
    using T = decltype(ex::just(1) | ex::bulk_unchunked(ex::seq, 3, [](int, int) {}));
    static_assert(std::is_same_v<ex::completion_signatures_of_t<T>,
                                 ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr)>>);
  }
  // Ill-formed calls.
  auto f = [](int) {};
  static_assert(!std::is_invocable_v<ex::bulk_t, decltype(ex::just()), int, int, decltype(f)>);
  static_assert(!std::is_invocable_v<ex::bulk_t, decltype(ex::just()), const ex::sequenced_policy&, double, decltype(f)>);
  static_assert(std::is_invocable_v<ex::bulk_t, decltype(ex::just()), const ex::sequenced_policy&, int, decltype(f)>);
  return 0;
}
