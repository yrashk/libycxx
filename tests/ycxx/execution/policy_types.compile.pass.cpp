// [execution.syn], [execpol.type], [execpol.seq], [execpol.par], [execpol.parunseq],
// [execpol.unseq], [execpol.objects]: <execution> declares the four policy classes in
// std::execution, the objects seq, par, par_unseq, unseq ("inline constexpr", so usable in
// constant expressions and of type const P), and is_execution_policy<T>, a
// Cpp17UnaryTypeTrait whose base characteristic is true_type exactly for the type of a standard
// (or implementation-defined) execution policy, with is_execution_policy_v<T> equal to its
// value.
#include <execution>
#include <type_traits>

namespace ex = std::execution;

static_assert(std::is_class_v<ex::sequenced_policy>);
static_assert(std::is_class_v<ex::parallel_policy>);
static_assert(std::is_class_v<ex::parallel_unsequenced_policy>);
static_assert(std::is_class_v<ex::unsequenced_policy>);

static_assert(std::is_same_v<decltype(ex::seq), const ex::sequenced_policy>);
static_assert(std::is_same_v<decltype(ex::par), const ex::parallel_policy>);
static_assert(std::is_same_v<decltype(ex::par_unseq), const ex::parallel_unsequenced_policy>);
static_assert(std::is_same_v<decltype(ex::unseq), const ex::unsequenced_policy>);

// the four are distinct types ("used as a unique type to disambiguate parallel algorithm
// overloading")
static_assert(!std::is_same_v<ex::sequenced_policy, ex::parallel_policy>);
static_assert(!std::is_same_v<ex::parallel_policy, ex::parallel_unsequenced_policy>);
static_assert(!std::is_same_v<ex::parallel_unsequenced_policy, ex::unsequenced_policy>);
static_assert(!std::is_same_v<ex::sequenced_policy, ex::unsequenced_policy>);

// constexpr objects: their addresses are constant expressions
constexpr const ex::sequenced_policy* ps = &ex::seq;
constexpr const ex::unsequenced_policy* pu = &ex::unseq;

// [execpol.type]/2 and [meta.rqmts]: Cpp17UnaryTypeTrait with base characteristic
static_assert(std::is_base_of_v<std::true_type, std::is_execution_policy<ex::sequenced_policy>>);
static_assert(std::is_base_of_v<std::true_type, std::is_execution_policy<ex::parallel_policy>>);
static_assert(std::is_base_of_v<std::true_type, std::is_execution_policy<ex::parallel_unsequenced_policy>>);
static_assert(std::is_base_of_v<std::true_type, std::is_execution_policy<ex::unsequenced_policy>>);
static_assert(std::is_base_of_v<std::false_type, std::is_execution_policy<int>>);
static_assert(std::is_base_of_v<std::false_type, std::is_execution_policy<void>>);
static_assert(std::is_base_of_v<std::false_type, std::is_execution_policy<std::true_type>>);
struct LooksLikeAPolicy {};
static_assert(std::is_base_of_v<std::false_type, std::is_execution_policy<LooksLikeAPolicy>>);
static_assert(std::is_execution_policy_v<ex::sequenced_policy>);
static_assert(std::is_execution_policy_v<ex::unsequenced_policy>);
static_assert(!std::is_execution_policy_v<int>);
static_assert(std::is_same_v<decltype(std::is_execution_policy_v<int>), const bool>);
static_assert(std::is_default_constructible_v<std::is_execution_policy<int>>);

int main() { return ps && pu ? 0 : 1; }
