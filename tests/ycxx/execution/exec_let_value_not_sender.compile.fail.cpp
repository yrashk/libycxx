// [exec.let]/9: check-types requires, for each value completion of the child, that the function
// returns a sender (is-valid-let-sender: sender<invoke_result_t<LetFn, decay_t<Ts>&...>>);
// otherwise it throws, so let_value(just(1), f) with an f returning int has no completion
// signatures and cannot be waited on ([exec.sync.wait]/1).
// EXPECT-ERROR: cannot complete|completion signature
#include <execution>
#include <utility>

namespace ex = std::execution;

void f() {
  auto s = ex::let_value(ex::just(1), [](int) { return 5; });
  (void)std::this_thread::sync_wait(std::move(s));
}

int main() { return 0; }
