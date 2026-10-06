// [exec.snd.transform]/5: "The exception specification is equivalent to noexcept(final-sndr)":
// when a domain's transform_sender may throw, so may transform_sender, and an exception it
// throws propagates to the caller (also out of connect, [exec.connect]/2, which transforms the
// sender with the receiver's environment first) instead of reaching a noexcept boundary.
// REQUIRES: exceptions
#include <execution>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;

struct snd {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>();
  }
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
struct thrown {};
struct throwing_domain {
  template <class S, class E>
    requires std::is_same_v<std::remove_cvref_t<S>, snd>
  static snd transform_sender(ex::start_t, S&&, const E&) {
    throw thrown{};
  }
};
using StartEnv = ex::prop<ex::get_domain_t, throwing_domain>;
static_assert(!noexcept(ex::transform_sender(std::declval<snd>(), StartEnv{})));
static_assert(noexcept(ex::transform_sender(std::declval<snd>(), ex::env<>{})));

int main() {
  bool caught = false;
  try {
    (void)ex::transform_sender(snd{}, StartEnv{});
  } catch (thrown) {
    caught = true;
  }
  CHECK(caught);

  exec_test::record<int> rec;
  caught = false;
  try {
    auto op = ex::connect(snd{}, exec_test::receiver_for(rec, StartEnv{}));
    ex::start(op);
  } catch (thrown) {
    caught = true;
  }
  CHECK(caught && rec.how == exec_test::done::none);
  return 0;
}
