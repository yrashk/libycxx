// A child's completion domain, reported through the adaptors' attributes, selects the
// customizations:
//   [exec.snd.transform]/2-4: transform_sender(sndr, env) uses completion-domain(sndr), the type
//     of get_completion_domain<>(get_env(sndr), env) ([exec.get.compl.domain]/2.2: the
//     set_value one), and repeats with each new sender's; [exec.connect]/2 transforms with the
//     receiver's environment;
//   [exec.sync.wait]/4: sync_wait(sndr) is apply_sender(Domain(), sync_wait, sndr), Domain the
//     sender's set_value completion domain in sync-wait-env ([exec.sync.wait.var]/1 likewise);
//   [exec.snd.general]/3: the domain of a sender's set_value completions: when_all's are its
//     children's ([exec.when.all]/15: COMMON-DOMAIN), let_value's those of the sender its
//     function returns ([exec.let]/10), then's its child's, continues_on's the scheduler's;
//   [exec.snd.expos]/8-9, [exec.domain.indeterminate]/3-4: a child that reports no domain does
//     not hide the others' (indeterminate_domain<> is the identity of COMMON-DOMAIN); two
//     different domains make an indeterminate_domain, whose transformations and apply_sender
//     are default_domain's ([exec.snd.apply]/1).
#include <execution>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;
using ex::set_value_t;

int then_transforms = 0, when_all_transforms = 0, waits = 0;

// Customizes then and when_all where they complete in this domain.
struct xform_domain {
  template <class S, class E>
    requires std::is_same_v<ex::tag_of_t<S>, ex::then_t>
  static auto transform_sender(set_value_t, S&&, const E&) noexcept {
    ++then_transforms;
    return ex::just(42);
  }
  template <class S, class E>
    requires std::is_same_v<ex::tag_of_t<S>, ex::when_all_t>
  static auto transform_sender(set_value_t, S&&, const E&) noexcept {
    ++when_all_transforms;
    return ex::just(7, 8);
  }
};
// Customizes sync_wait.
struct wait_domain {
  template <class S>
  static auto apply_sender(tt::sync_wait_t, S&&) {
    ++waits;
    using R = decltype(tt::sync_wait.apply_sender(std::declval<S>())); // the default's type ([exec.sync.wait]/5.3)
    return R();
  }
};

int started = 0;
// A sender whose value completions run in domain D.
template <class D, int N>
struct dsnd {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<set_value_t(int)>();
  }
  struct attrs {
    template <class... Env>
    D query(ex::get_completion_domain_t<set_value_t>, const Env&...) const noexcept {
      return {};
    }
  };
  attrs get_env() const noexcept { return {}; }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept {
      ++started;
      ex::set_value(std::move(r), N);
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r)};
  }
};
// A sender without attributes.
struct plain {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<set_value_t(int)>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept { ex::set_value(std::move(r), 5); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r)};
  }
};

int main() {
  using E0 = ex::env<>;
  // transform_sender through when_all's domain: two children in xform_domain.
  {
    auto s = ex::transform_sender(ex::when_all(dsnd<xform_domain, 1>(), dsnd<xform_domain, 2>()), E0());
    static_assert(std::is_same_v<decltype(s), decltype(ex::just(7, 8))>);
    CHECK(when_all_transforms == 1);
    // A child without a domain does not hide the other's.
    auto s2 = ex::transform_sender(ex::when_all(plain(), dsnd<xform_domain, 2>()), E0());
    static_assert(std::is_same_v<decltype(s2), decltype(ex::just(7, 8))>);
    // then over when_all: when_all's value domain is then's.
    auto s3 = ex::transform_sender(ex::when_all(dsnd<xform_domain, 1>(), dsnd<xform_domain, 2>()) | ex::then([](int a, int b) { return a + b; }), E0());
    static_assert(std::is_same_v<decltype(s3), decltype(ex::just(42))>);
  }
  // connect transforms: the customized then runs instead (sync_wait connects with its env).
  {
    then_transforms = 0;
    started = 0;
    // let_value's value completions are those of the sender f returns.
    auto [v] = tt::sync_wait(ex::let_value(ex::just(), [] { return dsnd<xform_domain, 1>(); }) | ex::then([](int x) { return x + 1; })).value();
    CHECK(v == 42 && then_transforms == 1 && started == 0);
    // when_all of two: the customized when_all.
    when_all_transforms = 0;
    auto [a, b] = tt::sync_wait(ex::when_all(dsnd<xform_domain, 1>(), dsnd<xform_domain, 2>())).value();
    CHECK(a == 7 && b == 8 && when_all_transforms == 1 && started == 0);
    // After continues_on the values complete on the run_loop's agents: default_domain.
    ex::run_loop loop;
    auto s = ex::continues_on(dsnd<xform_domain, 3>(), loop.get_scheduler()) | ex::then([](int x) { return x + 1; });
    static_assert(std::is_same_v<decltype(ex::get_completion_domain<set_value_t>(ex::get_env(s), E0())), ex::default_domain>);
    auto t = ex::transform_sender(std::move(s), E0());
    static_assert(std::is_same_v<ex::tag_of_t<decltype(t)>, ex::then_t>);
  }
  // sync_wait's customization, through when_all and let_value.
  {
    waits = 0;
    started = 0;
    auto r = tt::sync_wait(ex::when_all(dsnd<wait_domain, 1>(), dsnd<wait_domain, 2>()));
    static_assert(std::is_same_v<decltype(r), std::optional<std::tuple<int, int>>>);
    CHECK(!r && waits == 1 && started == 0);
    auto r2 = tt::sync_wait(ex::let_value(ex::just(), [] { return dsnd<wait_domain, 1>(); }));
    CHECK(!r2 && waits == 2);
    auto r3 = tt::sync_wait(ex::when_all(plain(), dsnd<wait_domain, 1>()) | ex::then([](int a, int b) { return a + b; }));
    CHECK(!r3 && waits == 3 && started == 0);
    // Two different domains: indeterminate_domain, which applies sync_wait as default_domain.
    using W = decltype(ex::when_all(dsnd<wait_domain, 1>(), dsnd<xform_domain, 2>() | ex::then([](int x) { return x; })));
    static_assert(std::is_same_v<decltype(ex::get_completion_domain<set_value_t>(ex::get_env(std::declval<W>()), E0())),
                                 ex::indeterminate_domain<wait_domain, xform_domain>>);
    auto [p, q] = tt::sync_wait(ex::when_all(dsnd<wait_domain, 1>(), plain())).value_or(std::tuple(0, 0));
    CHECK(p == 0 && q == 0 && waits == 4);
    auto [m, n] = tt::sync_wait(ex::when_all(dsnd<wait_domain, 1>(), ex::just(2) | ex::continues_on(ex::inline_scheduler()))).value();
    CHECK(m == 1 && n == 2 && waits == 4 && started == 1);
  }
  return 0;
}
