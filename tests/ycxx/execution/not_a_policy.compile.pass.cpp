// [algorithms.parallel.overloads]/4: "A parallel algorithm with a template parameter named
// ExecutionPolicy shall not participate in overload resolution unless that template parameter
// satisfies execution-policy", where [algorithms.parallel.defns]/2: execution-policy<Ep> is
// is_execution_policy_v<remove_cvref_t<Ep>>. So a call whose first argument is not a policy
// does not find a parallel overload (the expression is invalid, not a hard error), while the
// same call with a policy (lvalue, const lvalue or rvalue) is valid.
#include <algorithm>
#include <execution>
#include <numeric>
#include <utility>

struct NotAPolicy {};
int* p = nullptr;

template <class P>
concept copy4 = requires(P&& pol) { std::copy(std::forward<P>(pol), p, p, p); };
template <class P>
concept for_each4 = requires(P&& pol) { std::for_each(std::forward<P>(pol), p, p, [](int) {}); };
template <class P>
concept sort3 = requires(P&& pol) { std::sort(std::forward<P>(pol), p, p); };
template <class P>
concept reduce3 = requires(P&& pol) { std::reduce(std::forward<P>(pol), p, p); };
template <class P>
concept fill4 = requires(P&& pol) { std::fill(std::forward<P>(pol), p, p, 0); };

static_assert(!copy4<NotAPolicy>);
static_assert(!copy4<int>);
static_assert(!for_each4<NotAPolicy>);
static_assert(!sort3<NotAPolicy>);
static_assert(!reduce3<NotAPolicy>);
static_assert(!fill4<NotAPolicy>);

static_assert(copy4<const std::execution::sequenced_policy&>);
static_assert(copy4<std::execution::parallel_policy>);
static_assert(copy4<std::execution::parallel_unsequenced_policy&>);
static_assert(for_each4<const std::execution::unsequenced_policy&>);
static_assert(sort3<const std::execution::parallel_policy&>);
static_assert(reduce3<const std::execution::sequenced_policy&>);
static_assert(fill4<std::execution::unsequenced_policy&&>);

int main() { return 0; }
