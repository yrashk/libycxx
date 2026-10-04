// [sequence.reqmts]/9, 12, 38, 42, 59, 63, 111: "Each iterator in the range [i, j) is
// dereferenced exactly once" for X(i, j), a.insert(p, i, j) and a.assign(i, j), and "each
// iterator in the range rg is dereferenced exactly once" for X(from_range, rg),
// a.insert_range(p, rg), a.assign_range(rg) and a.append_range(rg). Checked with counting
// single-pass and forward iterators/ranges, inserting into the middle and with enough
// elements to require reallocation.
#include <vector>
#include <string>
#include <ranges>
#include "container_values.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

#include "reqs/sequence_single_pass.hpp"

using namespace reqs::sequence_single_pass;

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::vector<bool>>());
static_assert(test<std::string>());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<bool>>());
  CHECK(test<std::vector<std::string>>());
  CHECK(test<std::string>());
  CHECK(test<std::wstring>());
  return 0;
}
