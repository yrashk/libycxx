// [sequence.reqmts]/45-48: a.erase(q) has type iterator, erases the element q points to and
// returns an iterator to the element immediately following q prior to the erase, or a.end()
// if there is none. /49-52: a.erase(q1, q2) has type iterator, erases [q1, q2) and returns
// an iterator to the element q2 pointed to prior to the erase, or a.end(); an empty range
// erases nothing. /53-55: a.clear() has type void, destroys all elements, and
// a.empty() is true afterwards. q, q1, q2 are const_iterators.
#include <vector>
#include <string>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "check.hpp"

#include "reqs/sequence_erase_clear.hpp"

using namespace reqs::sequence_erase_clear;

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
  CHECK(test<std::u8string>());
  return 0;
}
