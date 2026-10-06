// [exec.snd.transform]/1-4: transform_sender(sndr, env) first transforms with the sender's
// completion domain and the tag set_value, repeating with the new sender's completion domain
// while the type changes, then with the environment's domain (get_domain(env)) and the tag
// start, repeating while the type changes; a domain without a matching transform_sender falls
// back to default_domain. [exec.domain.default]/2-4: default_domain::transform_sender is the
// sender's tag's transform_sender when there is one, else the sender itself
// (static_cast<Sndr>(std::forward<Sndr>(sndr)): an lvalue stays an lvalue reference);
// /5-8: default_domain::apply_sender is Tag().apply_sender(...). [exec.snd.apply]/1-3:
// apply_sender(dom, tag, sndr, args...) uses dom.apply_sender when it is well-formed, else the
// default domain's. The noexcept-ness follows the chosen expression.
#include <execution>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;

template <int N, class Attrs = ex::env<>>
struct snd {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>();
  }
  Attrs get_env() const noexcept { return Attrs{}; }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept { ex::set_value(std::move(r), N); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r)};
  }
};

int transforms = 0;
// The completion domain of snd<1>: turns it into snd<2>, whose completion domain turns it into
// snd<3> (the recursion of the set_value phase).
struct dom_b {
  template <class S, class E>
    requires std::is_same_v<std::remove_cvref_t<S>, snd<2, ex::prop<ex::get_completion_domain_t<ex::set_value_t>, dom_b>>>
  static auto transform_sender(ex::set_value_t, S&&, const E&) noexcept {
    ++transforms;
    return snd<3>{};
  }
};
struct dom_a {
  template <class S, class E>
    requires std::is_same_v<std::remove_cvref_t<S>, snd<1, ex::prop<ex::get_completion_domain_t<ex::set_value_t>, dom_a>>>
  static auto transform_sender(ex::set_value_t, S&&, const E&) {
    ++transforms;
    return snd<2, ex::prop<ex::get_completion_domain_t<ex::set_value_t>, dom_b>>{};
  }
};
// The start domain (from the environment): snd<3> becomes snd<4>.
struct dom_start {
  template <class S, class E>
    requires std::is_same_v<std::remove_cvref_t<S>, snd<3>>
  static auto transform_sender(ex::start_t, S&&, const E&) {
    ++transforms;
    return snd<4>{};
  }
  // apply_sender for a tag of our own.
  template <class Tag, class S, class... Args>
  static int apply_sender(Tag, S&&, Args&&... args) noexcept {
    return 100 + static_cast<int>(sizeof...(args));
  }
};

struct my_tag {
  template <class S, class... Args>
  static int apply_sender(S&&, Args&&...) {
    return 7;
  }
};

using A1 = snd<1, ex::prop<ex::get_completion_domain_t<ex::set_value_t>, dom_a>>;
using StartEnv = ex::prop<ex::get_domain_t, dom_start>;

static_assert(std::is_same_v<decltype(ex::transform_sender(A1{}, ex::env<>{})), snd<3>>);
static_assert(std::is_same_v<decltype(ex::transform_sender(A1{}, StartEnv{})), snd<4>>);
static_assert(std::is_same_v<decltype(ex::transform_sender(std::declval<snd<3>&>(), ex::env<>{})), snd<3>&>);
static_assert(std::is_same_v<decltype(ex::transform_sender(snd<3>{}, ex::env<>{})), snd<3>>);
static_assert(std::is_same_v<decltype(ex::default_domain().transform_sender(ex::set_value, std::declval<snd<5>&>(), ex::env<>{})), snd<5>&>);
static_assert(std::is_same_v<decltype(ex::default_domain().transform_sender(ex::start, std::declval<const snd<5>&>(), ex::env<>{})), const snd<5>&>);
static_assert(noexcept(ex::default_domain().transform_sender(ex::set_value, std::declval<snd<5>&>(), ex::env<>{})));
static_assert(!noexcept(ex::transform_sender(A1{}, ex::env<>{}))); // dom_a's transform is not noexcept
// apply_sender
static_assert(std::is_same_v<decltype(ex::apply_sender(ex::default_domain(), my_tag(), snd<1>{})), int>);
static_assert(!noexcept(ex::apply_sender(ex::default_domain(), my_tag(), snd<1>{})));
static_assert(noexcept(ex::apply_sender(dom_start(), my_tag(), snd<1>{}, 1, 2)));
template <class D, class T, class S>
concept can_apply = requires(D d, T t, S s) { ex::apply_sender(d, t, s); };
struct no_apply_tag {};
static_assert(!can_apply<ex::default_domain, no_apply_tag, snd<1>>); // [exec.domain.default]/6

int main() {
  transforms = 0;
  auto s = ex::transform_sender(A1{}, ex::env<>{});
  CHECK(transforms == 2);
  auto [v] = std::this_thread::sync_wait(std::move(s)).value();
  CHECK(v == 3);
  transforms = 0;
  auto s4 = ex::transform_sender(A1{}, StartEnv{});
  CHECK(transforms == 3);
  auto [w] = std::this_thread::sync_wait(std::move(s4)).value();
  CHECK(w == 4);
  // A sender no domain transforms is returned unchanged (and an lvalue by reference).
  snd<3> plain;
  CHECK(&ex::transform_sender(plain, ex::env<>{}) == &plain);
  // apply_sender: the domain's, else the tag's through default_domain.
  CHECK(ex::apply_sender(dom_start(), my_tag(), snd<1>{}, 1, 2, 3) == 103);
  CHECK(ex::apply_sender(dom_a(), my_tag(), snd<1>{}) == 7);
  CHECK(ex::default_domain().apply_sender(my_tag(), snd<1>{}, 0) == 7);
  return 0;
}
