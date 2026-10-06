// [task.promise]/4: yield_value(with_error<Err> err): "Mandates: std::move(err.error) is
// convertible to exactly one of the set_error_t argument types of error_types." An int converts
// to both long and double, so co_yield with_error(1) is ill-formed.
// EXPECT-ERROR: static assertion failed|no matching|no viable|ambiguous
#include <execution>

namespace ex = std::execution;

struct Env {
  using start_scheduler_type = ex::inline_scheduler;
  using error_types = ex::completion_signatures<ex::set_error_t(long), ex::set_error_t(double)>;
};

ex::task<int, Env> body() {
  co_yield ex::with_error(1);
  co_return 0;
}

void use() { (void)body(); }
