// [sequence.reqmts]/69.1-69.2: the InputIterator overloads of the (first, last[, alloc])
// constructor and of insert(p, first, last) / assign(first, last) "shall not participate in
// overload resolution" when InputIterator does not qualify as an input iterator, and
// [container.reqmts]/69: "as a minimum integral types shall not qualify as input iterators".
// So X(n, v), a.assign(n, v) and a.insert(p, n, v) with two integers of the same type select
// the (size_type, const T&) forms. /69.3: likewise the iterator-pair deduction guide does not
// participate, so vector(3, 4) deduces vector<int> through the (size_type, const T&)
// constructor.
#include <vector>
#include <string>
#include <type_traits>
#include "check.hpp"

#include "reqs/sequence_integral_dispatch.hpp"

using namespace reqs::sequence_integral_dispatch;

constexpr bool ctad() {
  std::vector v(3, 4);
  static_assert(std::is_same_v<decltype(v), std::vector<int>>);
  std::vector w(2u, 'c');
  static_assert(std::is_same_v<decltype(w), std::vector<char>>);
  return v.size() == 3 && v[1] == 4 && w.size() == 2 && w[0] == 'c';
}

static_assert(test<std::vector<int>, int>());
static_assert(test<std::vector<long>, long>());
static_assert(test<std::vector<unsigned>, unsigned>());
static_assert(test<std::vector<char>, char>());
static_assert(test<std::vector<long long>, short>());
static_assert(test<std::vector<double>, int>());
static_assert(test<std::string, int>());
static_assert(test<std::string, char>());
static_assert(ctad());

int main() {
  CHECK((test<std::vector<int>, int>()));
  CHECK((test<std::vector<long>, long>()));
  CHECK((test<std::vector<unsigned>, unsigned>()));
  CHECK((test<std::vector<char>, char>()));
  CHECK((test<std::vector<double>, int>()));
  CHECK((test<std::string, int>()));
  CHECK((test<std::u32string, long>()));
  CHECK(ctad());
  return 0;
}
