// [rand.dist.samp.pconst], [rand.dist.samp.plinear]: "vector<result_type> intervals() const;" and
// "vector<result_type> densities() const;", also for param_type ([rand.req.dist]/9: "identical
// name, type, and semantics"); [rand.dist.samp.discrete]: probabilities() returns vector<double>.
#include <random>
#include <type_traits>
#include <vector>

template <class X, class R>
constexpr bool vectors_of =
    std::is_same_v<decltype(std::declval<const X&>().intervals()), std::vector<R>> &&
    std::is_same_v<decltype(std::declval<const X&>().densities()), std::vector<R>>;

static_assert(vectors_of<std::piecewise_constant_distribution<float>, float>);
static_assert(vectors_of<std::piecewise_constant_distribution<float>::param_type, float>);
static_assert(vectors_of<std::piecewise_linear_distribution<float>, float>);
static_assert(vectors_of<std::piecewise_linear_distribution<float>::param_type, float>);
static_assert(vectors_of<std::piecewise_constant_distribution<long double>, long double>);
static_assert(vectors_of<std::piecewise_linear_distribution<long double>, long double>);
static_assert(std::is_same_v<decltype(std::declval<const std::discrete_distribution<long>&>().probabilities()),
                             std::vector<double>>);

int main() {}
