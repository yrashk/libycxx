// [hive.overview]/6, [hive.cons]: hive meets the container (except ==), reversible and
// allocator-aware container requirements. Construction from (n), (n, value), iterator
// ranges, ranges and initializer lists, each equal to its source; copy construction is
// equal to the source; move construction (noexcept) moves the element blocks, so pointers
// and iterators keep referring to the same elements; copy / move assignment and assign
// replace the contents; the allocator-extended forms use the given allocator; swap and
// propagate_on_container_* behave as for any allocator-aware container
// ([container.reqmts]/64, [container.alloc.reqmts]).
// REQUIRES: exceptions
#include <hive>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>
#include "test_allocators.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

template <class H>
std::vector<int> sorted(const H& h) {
  std::vector<int> v(h.begin(), h.end());
  std::sort(v.begin(), v.end());
  return v;
}

int main() {
  int arr[] = {4, 1, 3};
  std::hive<int> a(arr, arr + 3);
  CHECK(std::equal(a.begin(), a.end(), arr, arr + 3));
  std::hive<int> b(std::from_range, InputRange<int>{arr, arr + 3});
  CHECK(std::equal(b.begin(), b.end(), arr, arr + 3));
  std::hive<int> c{4, 1, 3};
  CHECK(std::equal(c.begin(), c.end(), arr, arr + 3));
  std::hive<int> d(std::size_t(4), 7);
  CHECK(d.size() == 4 && std::count(d.begin(), d.end(), 7) == 4);
  std::hive<int> z(std::size_t(3));
  CHECK(z.size() == 3 && std::count(z.begin(), z.end(), 0) == 3);
  std::hive<int> cp(a);
  CHECK(std::equal(cp.begin(), cp.end(), a.begin(), a.end()));
  int* p = &*a.begin();
  auto it = a.begin();
  std::hive<int> mv(std::move(a));
  CHECK(&*mv.begin() == p && it == mv.begin() && mv.size() == 3);
  std::hive<int> as;
  as = cp;
  CHECK(sorted(as) == sorted(cp));
  as = {9, 8};
  CHECK(sorted(as) == std::vector<int>({8, 9}));
  as.assign(arr, arr + 2);
  CHECK(sorted(as) == std::vector<int>({1, 4}));
  as.assign_range(std::vector<int>{5, 5, 5});
  CHECK(as.size() == 3);
  as.assign(std::size_t(2), 6);
  as.assign({1});
  CHECK(as.size() == 1 && *as.begin() == 1);
  std::hive<int> m2;
  m2 = std::move(mv);
  CHECK(&*m2.begin() == p && m2.size() == 3);
  std::hive<int> r{1, 2, 3};
  std::vector<int> back(r.rbegin(), r.rend());
  std::vector<int> fwd(r.begin(), r.end());
  std::reverse(back.begin(), back.end());
  CHECK(back == fwd);

  // allocators
  using A = IdAlloc<int, false, false, true>;
  std::hive<int, A> h1(A(1));
  h1.insert(1);
  std::hive<int, A> h2(h1, A(2));
  CHECK(h2.get_allocator().id == 2 && h2.size() == 1);
  std::hive<int, A> h3(std::move(h2), A(3));
  CHECK(h3.get_allocator().id == 3 && h3.size() == 1);
  std::hive<int, A> h4(std::size_t(2), 5, A(4));
  CHECK(h4.get_allocator().id == 4);
  std::hive<int, A> h5(A(5));
  h5 = h4;  // copy assignment does not propagate
  CHECK(h5.get_allocator().id == 5 && h5.size() == 2);
  std::hive<int, A> h6(A(6));
  h6.swap(h4);  // propagate_on_container_swap
  CHECK(h6.get_allocator().id == 4 && h4.get_allocator().id == 6 && h6.size() == 2 && h4.empty());
  std::hive<int, MinimalAlloc<int>> mh{3, 2, 1};
  CHECK(mh.size() == 3);
  return 0;
}
