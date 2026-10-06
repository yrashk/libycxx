// Environments seen by child operations:
//   [exec.write.env]: write_env(sndr, env) connects sndr with a receiver whose environment is
//     env joined to the receiver's (env's queries first).
//   [exec.unstoppable]: unstoppable(sndr) is write_env(sndr, prop(get_stop_token,
//     never_stop_token{})).
//   [exec.adapt.general]/3.4: an adaptor's children see FWD-ENV(get_env(rcvr)): the forwarding
//     queries of the receiver's environment, not the others.
//   [exec.adapt.general]/3.2: a single-child adaptor's attributes are its child's forwarding
//     attributes.
#include <execution>
#include <stop_token>
#include <type_traits>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

struct fwd_q_t : std::forwarding_query_t {
  template <class Self, class E>
    requires requires(const E& e, const Self& q) { e.query(q); }
  constexpr decltype(auto) operator()(this const Self& self, const E& e) noexcept {
    return e.query(self);
  }
};
inline constexpr fwd_q_t fwd_q{};
struct local_q_t {
  template <class Self, class E>
    requires requires(const E& e, const Self& q) { e.query(q); }
  constexpr decltype(auto) operator()(this const Self& self, const E& e) noexcept {
    return e.query(self);
  }
};
inline constexpr local_q_t local_q{};

// A sender whose attributes answer fwd_q and local_q.
struct attr_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>();
  }
  auto get_env() const noexcept { return ex::env{ex::prop(fwd_q, 1), ex::prop(local_q, 2)}; }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept { ex::set_value(std::move(r)); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r)};
  }
};

int main() {
  // write_env adds queries; its queries come first.
  {
    auto outer = ex::env{ex::prop(fwd_q, 10)};
    record<int, int> r;
    run(ex::write_env(ex::read_env(fwd_q), ex::prop(fwd_q, 20)), receiver_for(r, outer));
    CHECK(r.how == done::value && std::get<0>(*r.values) == 20);
    record<int, int> r2;
    run(ex::write_env(ex::read_env(local_q), ex::prop(local_q, 5)), receiver_for(r2));
    CHECK(r2.how == done::value && std::get<0>(*r2.values) == 5);
  }
  // Children of an adaptor see the forwarding queries of the receiver's environment only.
  {
    auto outer = ex::env{ex::prop(fwd_q, 3), ex::prop(local_q, 4)};
    record<int, int> r;
    run(ex::then(ex::read_env(fwd_q), [](int x) noexcept { return x; }), receiver_for(r, outer));
    CHECK(r.how == done::value && std::get<0>(*r.values) == 3);
    using S = decltype(ex::then(ex::read_env(local_q), [](int x) noexcept { return x; }));
    static_assert(!ex::sender_in<S, decltype(outer)>);
    static_assert(ex::sender_in<decltype(ex::read_env(local_q)), decltype(outer)>);
  }
  // unstoppable hides the receiver's stop token.
  {
    std::inplace_stop_source src;
    auto outer = ex::env{ex::prop(std::get_stop_token, src.get_token())};
    using S = decltype(ex::unstoppable(ex::read_env(std::get_stop_token)));
    static_assert(std::is_same_v<ex::value_types_of_t<S, decltype(outer)>, std::variant<std::tuple<std::never_stop_token>>>);
    record<int, std::never_stop_token> r;
    run(ex::unstoppable(ex::read_env(std::get_stop_token)), receiver_for(r, outer));
    CHECK(r.how == done::value);
  }
  // Attributes: an adaptor forwards its child's forwarding attributes.
  {
    auto s = ex::then(attr_sender{}, []() noexcept {});
    CHECK(fwd_q(ex::get_env(s)) == 1);
  }
  return 0;
}
