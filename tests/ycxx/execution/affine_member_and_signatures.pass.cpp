// [exec.affine]/2: affine(sndr) is ill-formed for a non-sender; it is a pipeable adaptor.
// /3: affine(sndr) is make-sender(affine, env<>(), sndr). /5: transform_sender of it is the
// child's member affine() when that is well-formed, else continues_on(child,
// UNSTOPPABLE-SCHEDULER(get_start_scheduler(env))); /4: that scheduler's schedule sender is
// unstoppable(schedule(sch)), so a stop request does not stop the move back. /7: without a start
// scheduler in the receiver's environment, the completion signatures cannot be computed (the
// sender is not a sender_in that environment), for a child without an affine() member.
#include <execution>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using exec_test::done;

int member_calls = 0;
struct knows_its_place {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>();
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
  // /5: a sender that already completes where it started provides affine().
  auto affine() const {
    ++member_calls;
    return ex::just(55);
  }
};

template <class S>
concept affine_ok = requires(S s) { ex::affine(s); };
static_assert(!affine_ok<int> && affine_ok<decltype(ex::just())>);
using StartEnv = ex::prop<ex::get_start_scheduler_t, ex::inline_scheduler>;
using A = decltype(ex::affine(knows_its_place{}));
static_assert(ex::sender<A>);
static_assert(ex::sender_in<A, StartEnv>);
// /7: without a start scheduler the signatures of affine(sndr) cannot be computed, for a child
// without an affine() member. (For a child with one, /5 makes transform_sender, which
// get_completion_signatures applies first, the member's result whatever the environment, which
// contradicts /7; not checked here.)
static_assert(!ex::sender_in<decltype(ex::affine(ex::just(1) | ex::then([](int v) { return v; }))), ex::env<>>);
static_assert(ex::sender_in<decltype(ex::affine(ex::just(1) | ex::then([](int v) { return v; }))), StartEnv>);
static_assert(std::is_same_v<decltype(ex::just(1) | ex::affine), decltype(ex::affine(ex::just(1)))>);

int main() {
  // /5: the member is used.
  member_calls = 0;
  auto t = ex::transform_sender(ex::affine(knows_its_place{}), StartEnv{});
  static_assert(std::is_same_v<decltype(t), decltype(ex::just(55))>);
  CHECK(member_calls == 1);
  {
    exec_test::record<int, int> rec;
    exec_test::run(ex::affine(knows_its_place{}), exec_test::receiver_for(rec, StartEnv{}));
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 55);
  }

  // /4-5 otherwise: continues_on the start scheduler, unstoppable. A run_loop is the start
  // scheduler; the child completes inline; the receiver's stop is already requested.
  {
    ex::run_loop loop;
    std::inplace_stop_source src;
    src.request_stop();
    auto env = ex::env{ex::prop(ex::get_start_scheduler, loop.get_scheduler()), ex::prop(std::get_stop_token, src.get_token())};
    exec_test::record<int, int> rec;
    auto op = ex::connect(ex::affine(ex::just(9) | ex::then([](int v) { return v + 1; })), exec_test::receiver_for(rec, env));
    ex::start(op);
    CHECK(rec.how == done::none); // waits for the loop
    loop.finish();
    loop.run();
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 10);
  }
  return 0;
}
