// [deque.overview]/2: deque meets the sequence container requirements, including the optional ones.
// The generic check support/reqs/sequence_range_kinds.hpp: X(from_range, rg), assign_range,
// insert_range, append_range and prepend_range accept every container-compatible-range ([container.intro.reqmts]/2):
// sized input-only ranges, approximately-sized ranges ([range.approximately.sized]) whose
// reserve_hint is wrong in either direction, ranges of a convertible type or of prvalues,
// move-only and non-const-iterable views, and (for a move-only T) views::as_rvalue and
// prvalue-producing transforms ([sequence.reqmts]/11-14, /40-43, /60-64, /109-111).
#include <deque>
#include "reqs/sequence_range_kinds.hpp"
#include "check.hpp"

template <class T>
using C = std::deque<T>;


int main() {
  CHECK(reqs::sequence_range_kinds::copyable_elements<C>());
  CHECK(reqs::sequence_range_kinds::move_only_elements<C>());
  return 0;
}
