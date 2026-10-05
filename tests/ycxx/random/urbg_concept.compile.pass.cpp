// [rand.req.urng]/1: uniform_random_bit_generator<G> = invocable<G&> &&
// unsigned_integral<invoke_result_t<G&>> && requires { { G::min() } -> same_as<invoke_result_t<G&>>;
// { G::max() } -> same_as<invoke_result_t<G&>>; requires bool_constant<(G::min() < G::max())>::value; }
// COUNTERPART: libcxx:numerics/rand/rand.req/rand.req.urng/uniform_random_bit_generator.compile.pass.cpp
#include <random>
#include <cstdint>

struct good {
  using result_type = unsigned;
  static constexpr unsigned min() { return 0; }
  static constexpr unsigned max() { return 10; }
  unsigned operator()() { return 1; }
};
struct signed_result {
  static constexpr int min() { return 0; }
  static constexpr int max() { return 10; }
  int operator()() { return 1; }
};
struct bool_result {
  static constexpr bool min() { return false; }
  static constexpr bool max() { return true; }
  bool operator()() { return true; }
};
struct min_not_less {
  static constexpr unsigned min() { return 5; }
  static constexpr unsigned max() { return 5; }
  unsigned operator()() { return 5; }
};
struct mismatched_min_type {
  static constexpr unsigned long min() { return 0; }
  static constexpr unsigned max() { return 10; }
  unsigned operator()() { return 1; }
};
struct nonstatic_min {
  constexpr unsigned min() const { return 0; }
  static constexpr unsigned max() { return 10; }
  unsigned operator()() { return 1; }
};
struct non_constexpr_bounds {
  static unsigned min() { return 0; }
  static unsigned max() { return 10; }
  unsigned operator()() { return 1; }
};
struct const_only_call {  // invocable<G&> only needs a call through an lvalue
  static constexpr std::uint8_t min() { return 0; }
  static constexpr std::uint8_t max() { return 255; }
  std::uint8_t operator()() const { return 1; }
};
struct rvalue_only_call {
  static constexpr unsigned min() { return 0; }
  static constexpr unsigned max() { return 10; }
  unsigned operator()() && { return 1; }
};
struct no_result_type_needed {  // the concept itself does not look at result_type
  static constexpr unsigned long long min() { return 1; }
  static constexpr unsigned long long max() { return 2; }
  unsigned long long operator()() { return 1; }
};

static_assert(std::uniform_random_bit_generator<good>);
static_assert(!std::uniform_random_bit_generator<signed_result>);
static_assert(std::uniform_random_bit_generator<bool_result>);  // unsigned_integral<bool> holds
static_assert(!std::uniform_random_bit_generator<min_not_less>);
static_assert(!std::uniform_random_bit_generator<mismatched_min_type>);
static_assert(!std::uniform_random_bit_generator<nonstatic_min>);
static_assert(!std::uniform_random_bit_generator<non_constexpr_bounds>);
static_assert(std::uniform_random_bit_generator<const_only_call>);
static_assert(!std::uniform_random_bit_generator<rvalue_only_call>);
static_assert(std::uniform_random_bit_generator<no_result_type_needed>);
static_assert(std::uniform_random_bit_generator<std::random_device>);
static_assert(std::uniform_random_bit_generator<std::mt19937>);
static_assert(!std::uniform_random_bit_generator<int>);
static_assert(!std::uniform_random_bit_generator<unsigned (*)()>);

int main() {}
