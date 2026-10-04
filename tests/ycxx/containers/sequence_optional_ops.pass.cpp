// [sequence.reqmts]/70-128, the operations required for basic_string and vector:
// a.front() / a.back() return *a.begin() / *--a.end() (const_reference for a const X,
// reference otherwise); a.push_back(t) and a.push_back(rv) append a copy (type void);
// a.pop_back() destroys the last element (type void); a[n] is *(a.begin() + n); a.at(n) is
// *(a.begin() + n) and throws out_of_range if n >= a.size(). Also append_range(rg) (type
// void, inserts copies of rg before end()), required for vector ([sequence.reqmts]/112) and
// provided by basic_string ([string.append]; there it returns *this).
#include <vector>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

#include "reqs/sequence_optional_ops.hpp"

using namespace reqs::sequence_optional_ops;

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
  CHECK(at_throws<std::vector<int>>());
  CHECK(at_throws<std::vector<Elem>>());
  CHECK(at_throws<std::vector<bool>>());
  CHECK(at_throws<std::string>());
  return 0;
}
