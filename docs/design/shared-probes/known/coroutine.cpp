// The compiler names std::coroutine_traits and std::coroutine_handle (and std::noop_coroutine is a
// builtin's caller); std::generator on top.
#include <coroutine>
#include <generator>
#include <cstdio>
struct task {
  struct promise_type {
    int value = 0;
    task get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_value(int v) { value = v; }
    void unhandled_exception() {}
  };
  std::coroutine_handle<promise_type> h;
};
task t() { co_return 42; }
std::generator<int> g() { co_yield 1; co_yield 2; }
int main() {
  task x = t();
  x.h.resume();
  int s = x.h.promise().value;
  x.h.destroy();
  for (int i : g()) s += i;
  std::noop_coroutine().resume();
  std::puts(s == 45 ? "ok" : "FAIL");
  return s == 45 ? 0 : 1;
}
