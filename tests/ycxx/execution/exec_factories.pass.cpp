// The sender factories ([exec.factories]):
//   [exec.just]: just(ts...), just_error(e), just_stopped() complete synchronously in start with
//     set_value(ts...), set_error(e), set_stopped(); their completion signatures are those of the
//     decayed arguments; just_error takes exactly one argument, just_stopped none; arguments must
//     be movable-value (not arrays).
//   [exec.read.env]: read_env(q) completes with set_value(q(get_env(rcvr))); it is a dependent
//     sender (its signatures need the environment).
//   [exec.schedule]: schedule(sch) is sch.schedule().
//   [exec.connect]: connect(sndr, rcvr) of a sender with a connect member; the receiver must
//     accept every completion (Mandates).
#include <execution>
#include <memory>
#include <stop_token>
#include <string>
#include <type_traits>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

struct move_only {
  int v;
  explicit move_only(int x) : v(x) {}
  move_only(move_only&&) = default;
  move_only& operator=(move_only&&) = default;
};

int main() {
  // just: the value channel, decayed signatures.
  {
    const std::string s = "abc";
    auto j = ex::just(1, s, 2.5);
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(j)>,
                                 ex::completion_signatures<ex::set_value_t(int, std::string, double)>>);
    record<int, int, std::string, double> r;
    run(j, receiver_for(r));
    CHECK(r.how == done::value && r.calls == 1);
    CHECK(std::get<1>(*r.values) == "abc");
    // The sender is copyable and connectable again (lvalue connect).
    record<int, int, std::string, double> r2;
    run(j, receiver_for(r2));
    CHECK(r2.how == done::value && std::get<0>(*r2.values) == 1);
  }
  // just of a move-only value: an rvalue sender only.
  {
    auto j = ex::just(move_only(5));
    static_assert(!std::is_copy_constructible_v<decltype(j)>);
    record<int, move_only> r;
    run(std::move(j), receiver_for(r));
    CHECK(r.how == done::value && std::get<0>(*r.values).v == 5);
  }
  // just_error / just_stopped
  {
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(ex::just_error(3))>,
                                 ex::completion_signatures<ex::set_error_t(int)>>);
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(ex::just_stopped())>, ex::completion_signatures<ex::set_stopped_t()>>);
    record<int> r;
    run(ex::just_error(3), receiver_for(r));
    CHECK(r.how == done::error && *r.error == 3);
    record<int> r2;
    run(ex::just_stopped(), receiver_for(r2));
    CHECK(r2.how == done::stopped && r2.calls == 1);
  }
  // Ill-formed calls ([exec.just]/2).
  static_assert(!std::is_invocable_v<ex::just_error_t>);
  static_assert(!std::is_invocable_v<ex::just_error_t, int, int>);
  static_assert(!std::is_invocable_v<ex::just_stopped_t, int>);
  static_assert(std::is_invocable_v<ex::just_t>);
  // Arrays are not movable-value.
  int arr[2] = {};
  static_assert(!std::is_invocable_v<ex::just_t, decltype((arr))>);
  (void)arr;


  // read_env: reads the receiver's environment.
  {
    std::inplace_stop_source src;
    auto env = ex::env{ex::prop(std::get_stop_token, src.get_token())};
    record<int, std::inplace_stop_token> r;
    run(ex::read_env(std::get_stop_token), receiver_for(r, env));
    CHECK(r.how == done::value && std::get<0>(*r.values) == src.get_token());
    // A query the environment cannot answer: no completion signatures.
    static_assert(!ex::sender_in<decltype(ex::read_env(ex::get_scheduler)), ex::env<>>);
    // A default: get_stop_token of an empty environment.
    static_assert(std::is_same_v<ex::value_types_of_t<decltype(ex::read_env(std::get_stop_token)), ex::env<>>,
                                 std::variant<std::tuple<std::never_stop_token>>>);
  }

  // schedule(sch) is sch.schedule(); the inline scheduler completes in start.
  {
    ex::inline_scheduler sch;
    auto s = ex::schedule(sch);
    static_assert(ex::sender<decltype(s)>);
    record<int> r;
    run(s, receiver_for(r));
    CHECK(r.how == done::value);
    CHECK(sch == ex::inline_scheduler());
  }

  // connect requires a receiver for every completion ([exec.connect]/6.9).
  struct value_only {
    using receiver_concept = ex::receiver_tag;
    void set_value(int) && noexcept {}
  };
  static_assert(std::is_same_v<decltype(ex::connect(ex::just(1), value_only{})), ex::connect_result_t<decltype(ex::just(1)), value_only>>);
  static_assert(ex::operation_state<ex::connect_result_t<decltype(ex::just(1)), value_only>>);
  return 0;
}
