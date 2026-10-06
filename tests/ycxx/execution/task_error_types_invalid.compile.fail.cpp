// [task.class]/3: a program is ill-formed if error_types is not a specialization of
// completion_signatures or has a template argument not of the form set_error_t(E). Here
// Environment::error_types has a value signature.
// EXPECT-ERROR: static assert.*error_types
#include <execution>

namespace ex = std::execution;

struct bad_env {
  using start_scheduler_type = ex::inline_scheduler;
  using error_types = ex::completion_signatures<ex::set_error_t(int), ex::set_value_t(int)>;
};

ex::task<void, bad_env> body() { co_return; }

void f() { (void)body(); }
