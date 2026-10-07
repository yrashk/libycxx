// [common.iter.cust]/3: iter_swap(x, y): "Hardened preconditions: holds_alternative<I>(x.v_) and holds_alternative<I2>(y.v_) are each true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <iterator>
#include "violation.hpp"

int main() {
  int a[2] = {1, 2};
  using CI = std::common_iterator<int*, std::unreachable_sentinel_t>;
  CI x(a), y(std::unreachable_sentinel);
  about_to_violate("common_iterator_iter_swap_sentinel");
  std::ranges::iter_swap(x, y);
  never_reached();
}
