// [exec.getcomplsigs]: the cases of get_completion_signatures<Sndr, Env...>().
//   /2: at most one environment.
//   /1, /3.1: with an environment, the signatures are those of NewSndr, the sender
//     transform_sender(sndr, env) returns ([exec.snd.transform]: the environment's domain
//     transforms it with the tag start); [exec.connect]/2 connects that same transformed sender.
//   /3.2: a sender whose static member get_completion_signatures takes only the sender type
//     is asked without the environment.
//   /1 CHECKED-COMPLSIGS: a member that does not return a completion_signatures specialization
//     makes the call exit with an exception: no signatures, with or without an environment.
//   /3.4-3.5: a sender with no member and not awaitable has no signatures: it is a dependent
//     sender without an environment (dependent_sender_error) and not sender_in with one.
//   /4: the signatures without an environment, when there are any, are a superset of those with one.
#include <execution>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

// /3.1: a dependent sender; the domain below turns it into just(42).
struct dep_s {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    if constexpr (sizeof...(Env) == 0)
      return (throw ex::dependent_sender_error(), ex::completion_signatures<ex::set_value_t()>());
    else
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
struct to_just_domain {
  template <class S, class E>
    requires std::is_same_v<std::remove_cvref_t<S>, dep_s>
  auto transform_sender(ex::start_t, S&&, const E&) const noexcept {
    return ex::just(42);
  }
};
using DomEnv = ex::env<ex::prop<ex::get_domain_t, to_just_domain>>;
static_assert(std::is_same_v<ex::completion_signatures_of_t<dep_s, ex::env<>>, ex::completion_signatures<ex::set_value_t()>>);
static_assert(std::is_same_v<ex::completion_signatures_of_t<dep_s, DomEnv>, ex::completion_signatures<ex::set_value_t(int)>>);
static_assert(std::is_same_v<decltype(ex::get_completion_signatures<dep_s, DomEnv>()), ex::completion_signatures<ex::set_value_t(int)>>);

// /2
template <class S, class... E>
concept callable_with = requires { ex::get_completion_signatures<S, E...>(); };
static_assert(callable_with<dep_s, ex::env<>>);
static_assert(!callable_with<dep_s, ex::env<>, ex::env<>>);
static_assert(!ex::sender_in<dep_s, ex::env<>, ex::env<>>);

// /3.2
struct self_only {
  using sender_concept = ex::sender_tag;
  template <class Self>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(char), ex::set_stopped_t()>();
  }
};
static_assert(ex::sender_in<self_only> && !ex::dependent_sender<self_only>);
static_assert(std::is_same_v<ex::completion_signatures_of_t<self_only, DomEnv>,
                             ex::completion_signatures<ex::set_value_t(char), ex::set_stopped_t()>>);

// /1: not a completion_signatures specialization.
struct bad_result {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval int get_completion_signatures() {
    return 0;
  }
};
static_assert(ex::sender<bad_result>);
static_assert(!ex::sender_in<bad_result>);
static_assert(!ex::sender_in<bad_result, ex::env<>>);

// /3.4-3.5: no member at all.
struct no_member {
  using sender_concept = ex::sender_tag;
};
static_assert(ex::sender<no_member>);
static_assert(ex::dependent_sender<no_member>);
static_assert(!ex::sender_in<no_member>);
static_assert(!ex::sender_in<no_member, ex::env<>>);

// /4: a library sender: with an environment the set is the same or smaller (here: equal).
static_assert(same_sigs<ex::completion_signatures_of_t<decltype(ex::just(1) | ex::then([](int) { return 2.0; }))>,
                        ex::completion_signatures_of_t<decltype(ex::just(1) | ex::then([](int) { return 2.0; })), DomEnv>>);

int main() {
  // [exec.connect]/2: connect uses the transformed sender.
  record<int, int> rec;
  run(dep_s{}, receiver_for(rec, DomEnv{ex::prop(ex::get_domain, to_just_domain())}));
  CHECK(rec.how == done::value && std::get<0>(*rec.values) == 42);
  // Without the domain, the sender itself.
  record<int> rec2;
  run(dep_s{}, receiver_for(rec2));
  CHECK(rec2.how == done::value);
  return 0;
}
