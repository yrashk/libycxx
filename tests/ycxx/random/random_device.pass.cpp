// [rand.device]: result_type is unsigned int; min() and max() are the limits of unsigned int;
// entropy() is noexcept and returns 0.0 or a value in [min(), log2(max() + 1)]; random_device is
// a uniform random bit generator and neither copyable nor assignable.
#include <random>
#include <limits>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<std::random_device::result_type, unsigned int>);
static_assert(std::random_device::min() == 0u);
static_assert(std::random_device::max() == std::numeric_limits<unsigned>::max());
static_assert(std::uniform_random_bit_generator<std::random_device>);
static_assert(!std::is_copy_constructible_v<std::random_device>);
static_assert(!std::is_copy_assignable_v<std::random_device>);
static_assert(std::is_default_constructible_v<std::random_device>);
static_assert(noexcept(std::declval<const std::random_device&>().entropy()));
static_assert(std::is_same_v<decltype(std::declval<const std::random_device&>().entropy()), double>);

int main() {
  std::random_device rd;
  double e = rd.entropy();
  CHECK(e >= 0.0 && e <= double(std::numeric_limits<unsigned>::digits));
  unsigned first = rd(), all_or = first, all_and = first;
  bool differs = false;
  for (int i = 0; i < 1000; ++i) {
    unsigned v = rd();
    differs = differs || v != first;
    all_or |= v;
    all_and &= v;
  }
  CHECK(differs);
  CHECK(all_or == ~0u && all_and == 0u);  // 1000 uniform values cover every bit both ways
  std::uniform_int_distribution<> d(1, 6);
  int v = d(rd);
  CHECK(1 <= v && v <= 6);
}
