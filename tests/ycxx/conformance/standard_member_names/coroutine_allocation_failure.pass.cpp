// A name a program's promise type spells for the core language to look up, with the library's
// coroutine support (DECISIONS §2: tools/data/uglify/allowed.txt keeps it, draft-names.txt reads
// only the library's clauses). [dcl.fct.def.coroutine]/10: when the promise type declares
// get_return_object_on_allocation_failure, the allocation function is the non-throwing form and
// a coroutine whose allocation returns a null pointer returns
// T::get_return_object_on_allocation_failure() without running its body. [coroutine.handle]:
// from_promise, done(), destroy(); [coroutine.trivial.awaitables]: suspend_always.
#include <coroutine>
#include <cstddef>
#include <new>
#include "check.hpp"

bool fail_allocation = false;
int bodies = 0;

struct task {
  struct promise_type {
    static void* operator new(std::size_t n) noexcept { return fail_allocation ? nullptr : ::operator new(n, std::nothrow); }
    static void operator delete(void* p) noexcept { ::operator delete(p); }
    static task get_return_object_on_allocation_failure() { return task{}; }
    task get_return_object() { return task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
  std::coroutine_handle<promise_type> handle;
};

task count_body() {
  ++bodies;
  co_return;
}

int main() {
  task t = count_body();
  CHECK(t.handle && !t.handle.done());
  t.handle.resume();
  CHECK(t.handle.done() && bodies == 1);
  t.handle.destroy();

  fail_allocation = true;
  task failed = count_body();
  CHECK(!failed.handle && bodies == 1);
  return 0;
}
