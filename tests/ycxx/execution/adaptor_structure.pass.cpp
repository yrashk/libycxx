// The senders some adaptors are specified to be ([exec.snd.expos]/45: a library sender is usable
// as the initializer of a structured binding [tag, data, children...]; [exec.snd.concepts]/6):
//   [exec.continues.on]/3: continues_on(sndr, sch) is make-sender(continues_on, sch,
//     schedule_from(sndr)): its data is sch and its child a schedule_from sender;
//   [exec.schedule.from]/1: schedule_from(sndr) is make-sender(schedule_from, {}, sndr), and it
//     is ill-formed for a non-sender;
//   [exec.starts.on]/3: starts_on(sch, sndr) is make-sender(starts_on, sch, sndr);
//   [exec.unstoppable]/2: unstoppable(sndr) is write_env(sndr, prop(get_stop_token,
//     never_stop_token{})) (expression-equivalent: the same type);
//   [exec.write.env]/2: write_env(sndr, env) is make-sender(write_env, env, sndr), ill-formed
//     for a non-sender.
#include <execution>
#include <stop_token>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;

template <class S>
auto tag_of_sender(const S& s) {
  auto&& [tag, data, child] = s;
  return tag;
}
template <class S>
const auto& data_of(const S& s) {
  auto&& [tag, data, child] = s;
  return data;
}
template <class S>
const auto& child_of(const S& s) {
  auto&& [tag, data, child] = s;
  return child;
}

int main() {
  ex::run_loop loop;
  auto sch = loop.get_scheduler();
  // continues_on
  {
    auto s = ex::continues_on(ex::just(1), sch);
    static_assert(std::is_same_v<decltype(tag_of_sender(s)), ex::continues_on_t>);
    CHECK(data_of(s) == sch);
    static_assert(std::is_same_v<decltype(tag_of_sender(child_of(s))), ex::schedule_from_t>);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(child_of(s))>, decltype(ex::schedule_from(ex::just(1)))>);
  }
  // schedule_from
  {
    static_assert(!std::is_invocable_v<ex::schedule_from_t, int>);
    auto s = ex::schedule_from(ex::just(2));
    static_assert(std::is_same_v<decltype(tag_of_sender(s)), ex::schedule_from_t>);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(child_of(s))>, decltype(ex::just(2))>);
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(s)>, ex::completion_signatures<ex::set_value_t(int)>>);
  }
  // starts_on
  {
    auto s = ex::starts_on(sch, ex::just(3));
    static_assert(std::is_same_v<decltype(tag_of_sender(s)), ex::starts_on_t>);
    CHECK(data_of(s) == sch);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(child_of(s))>, decltype(ex::just(3))>);
  }
  // unstoppable
  {
    using U = decltype(ex::unstoppable(ex::just(4)));
    using W = decltype(ex::write_env(ex::just(4), ex::prop(std::get_stop_token, std::never_stop_token{})));
    static_assert(std::is_same_v<U, W>);
    auto u = ex::unstoppable(ex::just(4));
    static_assert(std::is_same_v<decltype(tag_of_sender(u)), decltype(tag_of_sender(std::declval<W&>()))>);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::get_stop_token(data_of(u)))>, std::never_stop_token>);
  }
  // write_env
  {
    static_assert(std::is_invocable_v<decltype(ex::write_env), decltype(ex::just()), ex::env<>>);
    auto w = ex::write_env(ex::just(5), ex::env<>());
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(data_of(w))>, ex::env<>>);
    static_assert(!std::is_invocable_v<decltype(ex::write_env), int, ex::env<>>);
  }
  return 0;
}
