// [exec.bulk]/4: bulk.transform_sender(set_value, sndr, env) turns a bulk sender into a
// bulk_chunked sender (with the same child, policy and shape, and an f calling the original
// for each index of a chunk); it is ill-formed for a sender that is not a bulk sender. So
// transform_sender(bulk(...), env) ([exec.snd.transform]) gives a bulk_chunked sender with the
// default domain.
// /5, /7: an exception thrown by f is sent as set_error(exception_ptr) holding it (TRY-EVAL), and
// the value is then not sent; /9.2: errors and stopped of the child are forwarded without
// calling f; the policy may be any execution policy, also as an rvalue or a const lvalue.
// REQUIRES: exceptions
#include <execution>
#include <exception>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

// The tag of a sender, from its structured binding ([exec.snd.expos]/45, [exec.snd.concepts]/6).
template <class S>
auto tag_of_sender(S& s) {
  auto&& [tag, data, child] = s;
  return tag;
}
template <class S>
using tag_t = decltype(tag_of_sender(std::declval<S&>()));

template <class S>
constexpr bool tag_is(auto tag) {
  return std::is_same_v<tag_t<S>, decltype(tag)>;
}

template <class S>
concept bulk_transformable = requires(S&& s) { ex::bulk.transform_sender(ex::set_value, std::forward<S>(s), ex::env<>()); };

bool holds_int(const std::exception_ptr& e, int v) {
  try {
    std::rethrow_exception(e);
  } catch (int i) {
    return i == v;
  } catch (...) {
  }
  return false;
}

int main() {
  // /4
  {
    std::vector<int> hits(6, 0);
    auto b = ex::just(10) | ex::bulk(ex::par, 6, [&](int i, int& v) { hits[i] += v; });
    static_assert(tag_is<decltype(b)>(ex::bulk));
    static_assert(bulk_transformable<decltype(b)>);
    static_assert(bulk_transformable<decltype(b)&>);
    static_assert(!bulk_transformable<decltype(ex::just(10) | ex::bulk_chunked(ex::par, 6, [](int, int, int&) {}))>);
    static_assert(!bulk_transformable<decltype(ex::just(10))>);
    auto c = ex::bulk.transform_sender(ex::set_value, b, ex::env<>());
    static_assert(tag_is<decltype(c)>(ex::bulk_chunked));
    auto t = ex::transform_sender(std::move(b), ex::env<>());
    static_assert(tag_is<decltype(t)>(ex::bulk_chunked));
    // The chunked sender calls the original f once per index.
    record<int, int> rec;
    run(std::move(c), receiver_for(rec));
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 10);
    for (int h : hits)
      CHECK(h == 10);
  }
  // Exceptions from f, for each algorithm: the error holds f's exception; no value is sent.
  {
    record<std::exception_ptr, int> rec;
    run(ex::just(1) | ex::bulk(ex::seq, 4, [](int i, int) {
          if (i == 2)
            throw 12;
        }),
        receiver_for(rec));
    CHECK(rec.how == done::error && rec.calls == 1 && holds_int(*rec.error, 12) && !rec.values);
  }
  {
    record<std::exception_ptr, int> rec;
    run(ex::just(1) | ex::bulk_chunked(ex::seq, 4, [](int, int, int) { throw 13; }), receiver_for(rec));
    CHECK(rec.how == done::error && rec.calls == 1 && holds_int(*rec.error, 13) && !rec.values);
  }
  {
    record<std::exception_ptr, int> rec;
    run(ex::just(1) | ex::bulk_unchunked(ex::seq, 4, [](int i, int) {
          if (i == 3)
            throw 14;
        }),
        receiver_for(rec));
    CHECK(rec.how == done::error && rec.calls == 1 && holds_int(*rec.error, 14) && !rec.values);
  }
  // /9.2: stopped and errors are forwarded without calling f.
  {
    int calls = 0;
    auto f = [&](int) noexcept { ++calls; };
    auto fc = [&](int, int) noexcept { ++calls; };
    record<int> r1, r2, r3, r4;
    run(ex::just_stopped() | ex::bulk(ex::seq, 3, f), receiver_for(r1));
    run(ex::just_stopped() | ex::bulk_chunked(ex::seq, 3, fc), receiver_for(r2));
    run(ex::just_stopped() | ex::bulk_unchunked(ex::seq, 3, f), receiver_for(r3));
    run(ex::just_error(8) | ex::bulk_chunked(ex::seq, 3, fc), receiver_for(r4));
    CHECK(r1.how == done::stopped && r2.how == done::stopped && r3.how == done::stopped);
    CHECK(r4.how == done::error && *r4.error == 8);
    CHECK(calls == 0);
  }
  // Every standard policy, as an rvalue and as a const lvalue.
  {
    int sum = 0;
    const auto& pol = ex::par_unseq;
    record<int> rec;
    run(ex::just() | ex::bulk(ex::unseq, 3, [&](int i) { sum += i; }) | ex::bulk_unchunked(pol, 3, [&](int i) { sum += i; }) |
            ex::bulk_chunked(ex::parallel_policy(), 3, [&](int b, int e) { sum += e - b; }),
        receiver_for(rec));
    CHECK(rec.how == done::value && sum == 9);
  }
  return 0;
}
