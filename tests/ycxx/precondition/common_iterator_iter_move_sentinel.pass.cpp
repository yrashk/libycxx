// [common.iter.cust]/1: iter_move(i): "Hardened preconditions: holds_alternative<I>(i.v_) is true."
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
#include <iterator>
#include "violation.hpp"

int main() {
  using CI = std::common_iterator<int*, std::unreachable_sentinel_t>;
  CI it(std::unreachable_sentinel);
  about_to_violate("common_iterator_iter_move_sentinel");
  keep(std::ranges::iter_move(it));
  never_reached();
}
