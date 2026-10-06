// [task.promise]/7: unhandled_exception(): "If the signature set_error_t(exception_ptr) is not
// an element of error_types, calls terminate()". A task whose Environment's error_types has
// only set_error_t(int) and whose body throws ends the program through std::terminate (the
// handler below runs, then aborts).
// REQUIRES: exceptions
// EXPECT-TERMINATE: terminate handler called
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <execution>
#include <stdexcept>

namespace ex = std::execution;

struct Env {
  using start_scheduler_type = ex::inline_scheduler;
  using error_types = ex::completion_signatures<ex::set_error_t(int)>;
};

ex::task<int, Env> body() {
  throw std::runtime_error("escapes the coroutine");
  co_return 0;
}

struct rcvr {
  using receiver_concept = ex::receiver_tag;
  void set_value(int) && noexcept { std::puts("unexpected set_value"); }
  void set_error(int) && noexcept { std::puts("unexpected set_error"); }
  void set_stopped() && noexcept { std::puts("unexpected set_stopped"); }
};

int main() {
  std::set_terminate([] {
    std::puts("terminate handler called");
    std::fflush(stdout);
    std::abort();
  });
  static_assert(std::is_same_v<ex::completion_signatures_of_t<ex::task<int, Env>>,
                               ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(int), ex::set_stopped_t()>>);
  auto op = ex::connect(body(), rcvr{});
  ex::start(op);
  std::puts("start returned");
  return 0;
}
