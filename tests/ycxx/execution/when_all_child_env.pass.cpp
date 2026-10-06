// [exec.when.all]/6, /10: the environment of when_all's children is make-when-all-env(stop_src,
// get_env(rcvr)): get_stop_token gives the token of when_all's own inplace_stop_source (/6.2),
// a forwarding query is answered by the receiver's environment (/6.3), and a query that is not
// a forwarding query is ill-formed (/6.3), so a child that needs one makes the when_all sender
// fail to compute its completion signatures.
#include <execution>
#include <stop_token>
#include <tuple>
#include <type_traits>
#include <variant>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

// A query that forwards, and one that does not.
struct fwd_q_t {
  static constexpr bool query(std::forwarding_query_t) noexcept { return true; }
  template <class Env>
  constexpr auto operator()(const Env& e) const noexcept -> decltype(e.query(*this)) {
    return e.query(*this);
  }
};
inline constexpr fwd_q_t fwd_q{};

struct local_q_t {
  template <class Env>
  constexpr auto operator()(const Env& e) const noexcept -> decltype(e.query(*this)) {
    return e.query(*this);
  }
};
inline constexpr local_q_t local_q{};

static_assert(std::forwarding_query(fwd_q));
static_assert(!std::forwarding_query(local_q));

using Env = ex::env<ex::prop<fwd_q_t, int>, ex::prop<local_q_t, int>>;

// The receiver's environment answers both queries directly...
static_assert(ex::sender_in<decltype(ex::read_env(fwd_q)), Env>);
static_assert(ex::sender_in<decltype(ex::read_env(local_q)), Env>);
// ... but a when_all child sees only the forwarding one (/6.3).
static_assert(ex::sender_in<decltype(ex::when_all(ex::read_env(fwd_q))), Env>);
static_assert(!ex::sender_in<decltype(ex::when_all(ex::read_env(local_q))), Env>);
static_assert(!ex::sender_in<decltype(ex::when_all(ex::just(), ex::read_env(local_q))), Env>);
// The children's stop token is an inplace_stop_token whatever the receiver's (/6.2).
static_assert(std::is_same_v<ex::value_types_of_t<decltype(ex::when_all(ex::read_env(std::get_stop_token))), Env>,
                             std::variant<std::tuple<std::inplace_stop_token>>>);

int main() {
  Env env{ex::prop(fwd_q, 42), ex::prop(local_q, 7)};
  record<int, int, int> rec;
  run(ex::when_all(ex::read_env(fwd_q), ex::just(1)), receiver_for(rec, env));
  CHECK(rec.how == done::value && rec.calls == 1);
  CHECK(std::get<0>(*rec.values) == 42 && std::get<1>(*rec.values) == 1);

  // The receiver's token is a never_stop_token here; the children's can still be stopped.
  record<int, bool> rec2;
  run(ex::when_all(ex::read_env(std::get_stop_token) |
                   ex::then([](std::inplace_stop_token t) noexcept { return t.stop_possible(); })),
      receiver_for(rec2, env));
  CHECK(rec2.how == done::value && std::get<0>(*rec2.values));
  return 0;
}
