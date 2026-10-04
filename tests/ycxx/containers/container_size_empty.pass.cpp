// [container.reqmts]/52-62: c.size() has type size_type and returns distance(c.begin(),
// c.end()); c.max_size() has type size_type and returns distance(begin(), end()) for the
// largest possible container (so it is at least size() and, being a distance, at most the
// largest difference_type value); c.empty() has type bool and returns c.begin() == c.end().
// All three are usable on a const container.
#include <vector>
#include <string>
#include <iterator>
#include <limits>
#include <type_traits>
#include "container_values.hpp"
#include "check.hpp"

#include "reqs/container_size_empty.hpp"

using namespace reqs::container_size_empty;

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::vector<bool>>());
static_assert(test<std::string>());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<char>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<bool>>());
  CHECK(test<std::string>());
  CHECK(test<std::wstring>());
  CHECK(test<std::u32string>());
  return 0;
}
