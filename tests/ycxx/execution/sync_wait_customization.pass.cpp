// [exec.sync.wait]/4: sync_wait(sndr) is apply_sender(Domain(), sync_wait, sndr), Domain being
// the type of get_completion_domain<set_value_t>(get_env(sndr), sync-wait-env{}): a domain with
// an apply_sender for sync_wait_t replaces the default, which is sync_wait.apply_sender(sndr)
// (/10, through default_domain, [exec.domain.default]); /5.3 mandates that the customization
// returns sync-wait-result-type<Sndr>. [exec.sync.wait.var]/1, /3: the same for
// sync_wait_with_variant, whose default runs sync_wait(into_variant(sndr)).
#include <execution>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

int sync_wait_calls = 0, variant_calls = 0, started = 0;

struct my_domain {
  template <class S>
  std::optional<std::tuple<int>> apply_sender(tt::sync_wait_t, S&&) const {
    ++sync_wait_calls;
    return std::tuple<int>(77);
  }
  template <class S>
  std::optional<std::variant<std::tuple<int>>> apply_sender(tt::sync_wait_with_variant_t, S&&) const {
    ++variant_calls;
    return std::variant<std::tuple<int>>(std::tuple<int>(88));
  }
};

// A sender whose value completions belong to my_domain.
struct dom_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>();
  }
  struct attrs {
    template <class... Env>
    my_domain query(ex::get_completion_domain_t<ex::set_value_t>, const Env&...) const noexcept {
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
      ex::set_value(std::move(r), 1);
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r)};
  }
};

int main() {
  // The domain's apply_sender, not the default: the sender is not started.
  {
    auto r = tt::sync_wait(dom_sender());
    static_assert(std::is_same_v<decltype(r), std::optional<std::tuple<int>>>);
    CHECK(r && std::get<0>(*r) == 77 && sync_wait_calls == 1 && started == 0);
    auto v = tt::sync_wait_with_variant(dom_sender());
    static_assert(std::is_same_v<decltype(v), std::optional<std::variant<std::tuple<int>>>>);
    CHECK(v && std::get<0>(std::get<0>(*v)) == 88 && variant_calls == 1 && started == 0);
  }
  // The default: sync_wait.apply_sender(sndr) connects and runs.
  {
    auto r = tt::sync_wait.apply_sender(ex::just(3));
    static_assert(std::is_same_v<decltype(r), std::optional<std::tuple<int>>>);
    CHECK(r && std::get<0>(*r) == 3);
    auto v = tt::sync_wait_with_variant.apply_sender(ex::just(4));
    static_assert(std::is_same_v<decltype(v), std::optional<std::variant<std::tuple<int>>>>);
    CHECK(v && std::get<0>(std::get<0>(*v)) == 4);
  }
  // [exec.snd.general]/3: when_all's set_value completions happen where its children's do, so
  // its completion domain is my_domain too (COMMON-DOMAIN), and so is then's.
  {
    static_assert(std::is_same_v<decltype(ex::get_completion_domain<ex::set_value_t>(ex::get_env(ex::when_all(dom_sender())), ex::env<>())), my_domain>);
    auto r = tt::sync_wait(ex::when_all(dom_sender()));
    CHECK(r && std::get<0>(*r) == 77 && sync_wait_calls == 2 && started == 0);
    auto r2 = tt::sync_wait(dom_sender() | ex::then([](int x) noexcept { return x; }));
    CHECK(r2 && std::get<0>(*r2) == 77 && sync_wait_calls == 3 && started == 0);
  }
  return 0;
}
