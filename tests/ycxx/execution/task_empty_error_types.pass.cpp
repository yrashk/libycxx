// [task.class]/2: Environment::error_types is a specialization of completion_signatures whose
// signatures are set_error_t(E) -- including completion_signatures<>, a task that cannot complete
// with an error: its completion signatures are SET-VALUE-SIG(T) and set_stopped_t() alone, and an
// exception escaping the body calls terminate ([task.promise]/7, not run here). co_yield
// with_error(e) has no error type to convert to (ill-formed); the type with no error signature
// must still be usable. Likewise one whose only error type is not exception_ptr: a task that
// completes with that error (co_yield with_error) or a value, never set_error(exception_ptr).
// REQUIRES: exceptions
#include <execution>
#include <type_traits>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

struct no_errors {
  using error_types = ex::completion_signatures<>;
};

static_assert(std::is_same_v<ex::task<int, no_errors>::error_types, ex::completion_signatures<>>);
static_assert(same_sigs<ex::completion_signatures_of_t<ex::task<int, no_errors>>, ex::completion_signatures<ex::set_value_t(int), ex::set_stopped_t()>>);
static_assert(same_sigs<ex::completion_signatures_of_t<ex::task<void, no_errors>>, ex::completion_signatures<ex::set_value_t(), ex::set_stopped_t()>>);

ex::task<int, no_errors> value() { co_return 7; }
ex::task<int, no_errors> nested() {
  int v = co_await value();
  co_return v + 1;
}
ex::task<void, no_errors> stopped() { co_await ex::just_stopped(); }

struct int_error {
  using error_types = ex::completion_signatures<ex::set_error_t(int)>;
  using start_scheduler_type = ex::inline_scheduler;
};
static_assert(same_sigs<ex::completion_signatures_of_t<ex::task<int, int_error>>, ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(int), ex::set_stopped_t()>>);
ex::task<int, int_error> int_failure(bool fail) {
  if (fail)
    co_yield ex::with_error{3};
  co_return 4;
}

int main() {
  auto r = std::this_thread::sync_wait(nested());
  CHECK(r && std::get<0>(*r) == 8);
  CHECK(!std::this_thread::sync_wait(stopped()));
  {
    record<int, int> rec;
    run(int_failure(true), receiver_for(rec));
    CHECK(rec.how == done::error && *rec.error == 3);
    run(int_failure(false), receiver_for(rec));
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 4);
    // Through an adaptor whose result variant holds the task's signatures alone.
    auto r = std::this_thread::sync_wait(ex::continues_on(int_failure(false), ex::inline_scheduler{}));
    CHECK(r && std::get<0>(*r) == 4);
  }
  return 0;
}
