// [exec.sync.wait]/5.2: sync_wait(sndr) mandates that sync-wait-result-type<Sndr> is well-formed,
// i.e. that sndr has exactly one value completion signature in sync-wait-env (/3: value_types_of_t
// with type_identity_t as the variant). A sender with two value completions is ill-formed.
// EXPECT-ERROR: static assert.*sync_wait.*exactly one value completion
#include <execution>

namespace ex = std::execution;

struct two_values {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_value_t(double)>();
  }
};

void f() { (void)std::this_thread::sync_wait(two_values()); }
