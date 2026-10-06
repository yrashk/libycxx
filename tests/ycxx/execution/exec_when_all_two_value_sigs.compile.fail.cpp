// [exec.when.all]/9: check-types rejects a child whose completion signatures have two or more
// value completions (cs.count-of(set_value) >= 2 throws), so a when_all of such a child has no
// completion signatures and cannot be waited on ([exec.sync.wait]/1: sync_wait mandates
// sender_in<Sndr, sync-wait-env>).
// EXPECT-ERROR: cannot complete|completion signature
#include <execution>
#include <string>
#include <utility>

namespace ex = std::execution;

struct two_values {
  using sender_concept = ex::sender_tag;
  using completion_signatures = ex::completion_signatures<ex::set_value_t(int), ex::set_value_t(std::string)>;
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept { ex::set_value(std::move(r), 1); }
  };
  template <class R>
  op<R> connect(R r) && {
    return {std::move(r)};
  }
};

void f() {
  auto s = ex::when_all(two_values{}, ex::just());
  (void)std::this_thread::sync_wait(std::move(s));
}

int main() { return 0; }
