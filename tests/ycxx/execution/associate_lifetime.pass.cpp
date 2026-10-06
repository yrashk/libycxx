// [exec.associate]:
//   /11: connecting an associate sender moves its association into the operation state
//     (op_state::assoc), which owns it until the operation state is destroyed: a join of the
//     scope does not complete while that operation state exists, even after the operation
//     completed; connecting an lvalue copies the data first (/5: a new try_associate), so the
//     sender keeps its own association too;
//   /11 run(): an operation without an association completes with set_stopped, and the
//     completion signatures therefore include set_stopped_t() ([exec.snd.expos]/47) next to the
//     wrapped sender's;
//   /4: associate-data is copyable only when the wrapped sender is, and so is the sender;
//   /9.1: associate(sndr, token) needs a sender and a scope token.
#include <execution>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

using join_env = ex::env<ex::prop<ex::get_start_scheduler_t, ex::inline_scheduler>>;

struct move_only_sender {
  using sender_concept = ex::sender_tag;
  std::unique_ptr<int> p;
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
  op<R> connect(R r) && noexcept {
    return {std::move(r)};
  }
};

int main() {
  // The operation state owns the association.
  {
    ex::simple_counting_scope scope;
    auto s = ex::associate(ex::just(1), scope.get_token());
    record<int> join;
    auto jop = ex::connect(scope.join(), receiver_for(join, join_env()));
    record<int, int> rec;
    {
      auto op = ex::connect(std::move(s), receiver_for(rec));
      ex::start(op);
      CHECK(rec.how == done::value && std::get<0>(*rec.values) == 1);
      ex::start(jop);
      CHECK(join.how == done::none); // op still holds the association (s holds none now)
    }
    CHECK(join.how == done::value && join.calls == 1);
  }
  // Connecting an lvalue: the sender keeps its association; the operation gets a new one.
  {
    ex::simple_counting_scope scope;
    std::optional s(ex::associate(ex::just(2), scope.get_token()));
    record<int, int> rec;
    record<int> join;
    auto* raw = new auto(ex::connect(*s, receiver_for(rec)));
    std::unique_ptr<std::remove_pointer_t<decltype(raw)>> op(raw);
    auto jop = ex::connect(scope.join(), receiver_for(join, join_env()));
    ex::start(jop);
    ex::start(*op);
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 2);
    op.reset();
    CHECK(join.how == done::none); // the sender's own association remains
    s.reset();
    CHECK(join.how == done::value);
  }
  // Without an association: set_stopped; and the signatures say so.
  {
    using S = decltype(ex::associate(ex::just(3), std::declval<ex::simple_counting_scope::token>()));
    static_assert(same_sigs<ex::completion_signatures_of_t<S>, ex::completion_signatures<ex::set_value_t(int), ex::set_stopped_t()>>);
    ex::simple_counting_scope scope;
    scope.close();
    record<int, int> rec;
    run(ex::associate(ex::just(3), scope.get_token()), receiver_for(rec));
    CHECK(rec.how == done::stopped && rec.calls == 1);
    (void)std::this_thread::sync_wait(scope.join());
  }
  // /4
  {
    using C = decltype(ex::associate(ex::just(), std::declval<ex::counting_scope::token>()));
    static_assert(std::is_copy_constructible_v<C>);
    using M = decltype(ex::associate(move_only_sender(), std::declval<ex::counting_scope::token>()));
    static_assert(!std::is_copy_constructible_v<M> && std::is_move_constructible_v<M>);
    ex::counting_scope scope;
    record<int> rec;
    run(ex::associate(move_only_sender(), scope.get_token()), receiver_for(rec));
    CHECK(rec.how == done::value);
    (void)std::this_thread::sync_wait(scope.join());
  }
  // /9.1
  static_assert(!std::is_invocable_v<ex::associate_t, int, ex::simple_counting_scope::token>);
  static_assert(!std::is_invocable_v<ex::associate_t, decltype(ex::just()), int>);
  static_assert(std::is_invocable_v<ex::associate_t, decltype(ex::just()), ex::simple_counting_scope::token>);
  return 0;
}
