// [exec.inline.scheduler]:
//   /1: sch.query(q, args...) is inline-attrs<set_value_t>().query(q, args...)
//     ([exec.snd.expos]/60-61): with an environment, its set_value completion scheduler is
//     get_scheduler(env) and its completion domain get_domain(env); all inline_schedulers are
//     equal; schedule() is constexpr and noexcept;
//   [exec.sched]/6: the schedule sender's attributes answer the same;
//   /2: the schedule sender's completion signatures are set_value_t() alone;
//   /3: connect is potentially-throwing iff copying the receiver is; /4.2: start completes with
//     set_value at once.
#include <execution>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

struct my_domain {};

// A receiver whose copy can throw.
struct throwing_copy_receiver {
  using receiver_concept = ex::receiver_tag;
  bool* done;
  throwing_copy_receiver(bool* d) : done(d) {}
  throwing_copy_receiver(const throwing_copy_receiver& o) noexcept(false) : done(o.done) {}
  throwing_copy_receiver(throwing_copy_receiver&& o) noexcept : done(o.done) {}
  void set_value() && noexcept { *done = true; }
};

int main() {
  constexpr ex::inline_scheduler sch;
  static_assert(sch == ex::inline_scheduler());
  static_assert(noexcept(sch.schedule()));
  constexpr auto s = ex::inline_scheduler().schedule();
  (void)s;
  static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(ex::schedule(sch))>, ex::completion_signatures<ex::set_value_t()>>);

  ex::run_loop loop;
  auto env = ex::env{ex::prop(ex::get_scheduler, loop.get_scheduler()), ex::prop(ex::get_domain, my_domain())};
  // /1
  CHECK(ex::get_completion_scheduler<ex::set_value_t>(sch, env) == loop.get_scheduler());
  static_assert(std::is_same_v<decltype(ex::get_completion_domain<ex::set_value_t>(sch, env)), my_domain>);
  // [exec.sched]/6
  CHECK(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(ex::schedule(sch)), env) == loop.get_scheduler());
  static_assert(std::is_same_v<decltype(ex::get_completion_domain<ex::set_value_t>(ex::get_env(ex::schedule(sch)), env)), my_domain>);
  // Without a scheduler in the environment, the inline scheduler itself (/1 via
  // [exec.get.compl.sched]/5.2: it has no answer, and is a scheduler).
  CHECK(ex::get_completion_scheduler<ex::set_value_t>(sch, ex::env<>()) == sch);

  // /3, /4.2
  bool done = false;
  throwing_copy_receiver r(&done);
  static_assert(!noexcept(ex::connect(ex::schedule(sch), r)));
  static_assert(noexcept(ex::connect(ex::schedule(sch), std::move(r))));
  auto op = ex::connect(ex::schedule(sch), r);
  CHECK(!done);
  ex::start(op);
  CHECK(done);
  return 0;
}
