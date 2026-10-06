// [exec.when.all]/15, /17: how when_all completes when its children complete differently. The
// children of these tests complete synchronously, in order, when started (/16: start(ops)...).
//   - set_error exchanges the disposition to error unconditionally: the first error wins over
//     any later error and over an earlier stopped (/17);
//   - set_stopped changes the disposition only from started (/17), so it does not replace an
//     error;
//   - a value arriving after an error or a stop is not used (/17), and the result is the error
//     (/15.2) or stopped (/15.3);
//   - each completion requests a stop of the other children (/17), on error and on stopped.
//   - exactly one completion reaches the receiver.
#include <execution>
#include <stop_token>
#include <type_traits>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

// Completes with set_value(v), recording whether a stop was requested when it started.
struct probe {
  using sender_concept = ex::sender_tag;
  int v;
  bool* stop_seen;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    int v;
    bool* stop_seen;
    void start() & noexcept {
      *stop_seen = std::get_stop_token(ex::get_env(r)).stop_requested();
      ex::set_value(std::move(r), v);
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), v, stop_seen};
  }
};

int main() {
  // stopped, then error: the error.
  {
    record<int, int> rec;
    run(ex::when_all(ex::just_stopped(), ex::just_error(3)), receiver_for(rec));
    CHECK(rec.how == done::error && rec.calls == 1 && *rec.error == 3);
  }
  // error, then stopped: the error.
  {
    record<int, int> rec;
    run(ex::when_all(ex::just_error(4), ex::just_stopped()), receiver_for(rec));
    CHECK(rec.how == done::error && rec.calls == 1 && *rec.error == 4);
  }
  // Two errors: the first.
  {
    record<int, int> rec;
    run(ex::when_all(ex::just_error(5), ex::just_error(6), ex::just_error(7)), receiver_for(rec));
    CHECK(rec.how == done::error && rec.calls == 1 && *rec.error == 5);
  }
  // Errors of different types: the first, whatever its type; the signatures have both.
  {
    using S = decltype(ex::when_all(ex::just_error(1.5), ex::just_error(8)));
    static_assert(same_sigs<ex::completion_signatures_of_t<S>,
                            ex::completion_signatures<ex::set_error_t(double), ex::set_error_t(int)>>);
    record<double, int> rec;
    run(ex::when_all(ex::just_error(1.5), ex::just_error(8)), receiver_for(rec));
    CHECK(rec.how == done::error && rec.calls == 1 && *rec.error == 1.5);
  }
  // A value after an error: the error; the later child sees the stop request.
  {
    bool stop_seen = false;
    record<int, int> rec;
    run(ex::when_all(ex::just_error(9), probe{1, &stop_seen}), receiver_for(rec));
    CHECK(rec.how == done::error && *rec.error == 9 && stop_seen);
  }
  // A value after stopped: stopped; the later child sees the stop request.
  {
    bool stop_seen = false;
    record<int, int> rec;
    run(ex::when_all(ex::just_stopped(), probe{1, &stop_seen}), receiver_for(rec));
    CHECK(rec.how == done::stopped && rec.calls == 1 && stop_seen);
  }
  // Values before the failure are dropped: the error.
  {
    bool stop_seen = true;
    record<int, int> rec;
    run(ex::when_all(probe{1, &stop_seen}, ex::just_error(10)), receiver_for(rec));
    CHECK(!stop_seen);
    CHECK(rec.how == done::error && *rec.error == 10 && !rec.values);
  }
  // All values: the values, in the children's order.
  {
    bool s1 = true, s2 = true;
    record<int, int, int> rec;
    run(ex::when_all(probe{1, &s1}, probe{2, &s2}), receiver_for(rec));
    CHECK(rec.how == done::value && rec.calls == 1 && !s1 && !s2);
    CHECK(std::get<0>(*rec.values) == 1 && std::get<1>(*rec.values) == 2);
  }
  return 0;
}
