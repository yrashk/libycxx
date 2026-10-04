// [container.reqmts]/67: "Unless otherwise specified (either explicitly or by defining a
// function in terms of other functions), invoking a container member function or passing a
// container as an argument to a library function shall not invalidate iterators to, or
// change the values of, objects within that container." Checked for const observers,
// non-const element access and iteration, comparisons, and the [iterator.range] /
// [range.access] functions; afterwards every element is still at the same address with the
// same value. ([string.require]/4 lists operator[], at, data, front, back, begin, rbegin,
// end and rend as non-invalidating for basic_string, too.)
#include <vector>
#include <string>
#include <iterator>
#include <memory>
#include <ranges>
#include "container_values.hpp"
#include "check.hpp"

template <class X>
constexpr bool test() {
  using T = typename X::value_type;
  X a = make<X>({1, 2, 3, 4, 5});
  X other = make<X>({1, 2, 3});
  const T* addr[5];
  {
    int k = 0;
    for (auto& x : a) addr[k++] = std::addressof(x);
  }
  auto it = a.begin() + 2;
  (void)a.size(); (void)a.empty(); (void)a.max_size(); (void)a.capacity();
  (void)a.begin(); (void)a.end(); (void)a.cbegin(); (void)a.cend();
  (void)a.rbegin(); (void)a.rend(); (void)a.crbegin(); (void)a.crend();
  (void)a.front(); (void)a.back(); (void)a[1]; (void)a.at(3); (void)a.data();
  (void)a.get_allocator();
  (void)(a == other); (void)(a != other); (void)(other == a);
  (void)std::size(a); (void)std::ssize(a); (void)std::empty(a); (void)std::data(a);
  (void)std::begin(a); (void)std::end(a); (void)std::rbegin(a);
  (void)std::ranges::begin(a); (void)std::ranges::end(a); (void)std::ranges::size(a);
  (void)std::ranges::data(a); (void)std::ranges::cbegin(a);
  X copy(a);  // copying from a does not change a
  (void)copy;
  int k = 0;
  for (auto& x : a) {
    if (std::addressof(x) != addr[k] || !(x == val<T>(k + 1))) return false;
    ++k;
  }
  if (std::addressof(*it) != addr[2] || !(*it == val<T>(3))) return false;
  return k == 5;
}

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::string>());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<std::string>>());
  CHECK(test<std::string>());
  CHECK(test<std::wstring>());
  return 0;
}
