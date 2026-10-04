// [coro.generator.promise]: promise_type declares `void await_transform() = delete;`, so a
// co_await expression in a generator coroutine is ill-formed ([expr.await]/3.2: the
// await_transform lookup finds a declaration, and the call is ill-formed).
#include <coroutine>
#include <generator>

std::generator<int> g() {
#ifndef YCXX_CONTROL
  co_await std::suspend_never{};
#endif
  co_yield 1;
}

int main() {}
