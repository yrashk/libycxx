// The join sender of simple_counting_scope and counting_scope, and their members' signatures:
//   [exec.simple.counting.mem]/8: when the last association ends while joining, complete() is
//     called on every registered join operation: two joins started while associations are
//     held both complete;
//   [exec.counting.scopes.general]/4: a registered join completes by starting
//     schedule(get_start_scheduler(get_env(rcvr))) connected to rcvr-t, which forwards that
//     operation's completions: an error or stopped of the start scheduler reaches the join's
//     receiver; a join started while the count is 0 completes inline (complete-inline), also on
//     a scope already joined ([exec.simple.counting.mem]/9);
//   [exec.simple.counting.mem]/2.3, /3: close() while open-and-joining moves to
//     closed-and-joining: later associations fail and the join still completes;
//   [exec.simple.counting.token]/1: simple_counting_scope's wrap returns the same sender;
//   [exec.scope.counting]/7: counting_scope's wrap is noexcept iff the decay-copy of the sender is;
//   the members get_token, close, join and request_stop and the constructors are noexcept, and
//   the scopes are neither copyable nor movable ([exec.scope.simple.counting.general],
//   [exec.scope.counting]).
#include <execution>
#include <optional>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

// A scheduler whose schedule operation completes as told: 0 value, 1 error 99, 2 stopped.
struct flaky_scheduler {
  using scheduler_concept = ex::scheduler_tag;
  int mode = 0;
  struct sender {
    using sender_concept = ex::sender_tag;
    int mode;
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(int), ex::set_stopped_t()>();
    }
    template <class R>
    struct op {
      using operation_state_concept = ex::operation_state_tag;
      R r;
      int mode;
      void start() & noexcept {
        if (mode == 0)
          ex::set_value(std::move(r));
        else if (mode == 1)
          ex::set_error(std::move(r), 99);
        else
          ex::set_stopped(std::move(r));
      }
    };
    template <class R>
    op<R> connect(R r) const noexcept {
      return {std::move(r), mode};
    }
  };
  sender schedule() const noexcept { return {mode}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  bool operator==(const flaky_scheduler&) const = default;
};

using inline_env = ex::env<ex::prop<ex::get_start_scheduler_t, ex::inline_scheduler>>;
using flaky_env = ex::env<ex::prop<ex::get_start_scheduler_t, flaky_scheduler>>;

struct throwing_copy_sender {
  using sender_concept = ex::sender_tag;
  throwing_copy_sender() = default;
  throwing_copy_sender(const throwing_copy_sender&) noexcept(false) {}
  throwing_copy_sender(throwing_copy_sender&&) noexcept = default;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>();
  }
};

template <class Scope>
void check_scope() {
  static_assert(std::is_nothrow_default_constructible_v<Scope>);
  static_assert(!std::is_move_constructible_v<Scope> && !std::is_copy_constructible_v<Scope>);
  static_assert(noexcept(std::declval<Scope&>().get_token()));
  static_assert(noexcept(std::declval<Scope&>().close()));
  static_assert(noexcept(std::declval<Scope&>().join()));
  static_assert(ex::sender<decltype(std::declval<Scope&>().join())>);
  static_assert(same_sigs<ex::completion_signatures_of_t<decltype(std::declval<Scope&>().join()), inline_env>,
                          ex::completion_signatures<ex::set_value_t()>>);
  static_assert(same_sigs<ex::completion_signatures_of_t<decltype(std::declval<Scope&>().join()), flaky_env>,
                          ex::completion_signatures<ex::set_value_t(), ex::set_error_t(int), ex::set_stopped_t()>>);

  // Two joins registered; both complete when the last association ends.
  {
    Scope scope;
    std::optional a(scope.get_token().try_associate());
    std::optional b(scope.get_token().try_associate());
    CHECK(*a && *b);
    record<int> j1, j2;
    auto op1 = ex::connect(scope.join(), receiver_for(j1, inline_env()));
    auto op2 = ex::connect(scope.join(), receiver_for(j2, inline_env()));
    ex::start(op1);
    ex::start(op2);
    CHECK(j1.how == done::none && j2.how == done::none);
    a.reset();
    CHECK(j1.how == done::none && j2.how == done::none);
    // Still open-and-joining: associating works.
    std::optional c(scope.get_token().try_associate());
    CHECK(*c);
    b.reset();
    CHECK(j1.how == done::none);
    c.reset();
    CHECK(j1.how == done::value && j1.calls == 1 && j2.how == done::value && j2.calls == 1);
    // Joined: a new join completes inline; associations fail.
    record<int> j3;
    run(scope.join(), receiver_for(j3, inline_env()));
    CHECK(j3.how == done::value);
    CHECK(!scope.get_token().try_associate());
  }
  // A registered join completes through its start scheduler: its error, its stopped.
  for (int mode = 1; mode < 3; ++mode) {
    Scope scope;
    std::optional a(scope.get_token().try_associate());
    record<int> j;
    auto op = ex::connect(scope.join(), receiver_for(j, flaky_env{ex::prop(ex::get_start_scheduler, flaky_scheduler{mode})}));
    ex::start(op);
    CHECK(j.how == done::none);
    a.reset();
    CHECK(j.calls == 1);
    if (mode == 1)
      CHECK(j.how == done::error && *j.error == 99);
    else
      CHECK(j.how == done::stopped);
  }
  // ... but a join that completes inline does not schedule.
  {
    Scope scope;
    record<int> j;
    run(scope.join(), receiver_for(j, flaky_env{ex::prop(ex::get_start_scheduler, flaky_scheduler{1})}));
    CHECK(j.how == done::value);
  }
  // close() while joining.
  {
    Scope scope;
    std::optional a(scope.get_token().try_associate());
    record<int> j;
    auto op = ex::connect(scope.join(), receiver_for(j, inline_env()));
    ex::start(op);
    scope.close();
    CHECK(!scope.get_token().try_associate());
    CHECK(j.how == done::none);
    a.reset();
    CHECK(j.how == done::value);
  }
}

int main() {
  check_scope<ex::simple_counting_scope>();
  check_scope<ex::counting_scope>();
  static_assert(noexcept(std::declval<ex::counting_scope&>().request_stop()));
  // wrap
  {
    ex::simple_counting_scope s;
    auto j = ex::just(1);
    static_assert(std::is_same_v<decltype(s.get_token().wrap(j)), decltype(j)&>);
    static_assert(std::is_same_v<decltype(s.get_token().wrap(std::move(j))), decltype(j)&&>);
    CHECK(&s.get_token().wrap(j) == &j);
    static_assert(noexcept(s.get_token().wrap(j)));
    ex::counting_scope c;
    static_assert(noexcept(c.get_token().wrap(j)));
    throwing_copy_sender t;
    static_assert(!noexcept(c.get_token().wrap(t)));
    static_assert(noexcept(c.get_token().wrap(std::move(t))));
    static_assert(ex::sender<decltype(c.get_token().wrap(t))>);
  }
  return 0;
}
