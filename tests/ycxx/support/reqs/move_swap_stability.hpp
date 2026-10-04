// [container.reqmts]/15-16: "X u(rv);" is constant for all standard containers other than
// array and inplace_vector; [container.alloc.reqmts]/17,20: X u(rv) and X u(rv, m) with
// m == rv.get_allocator() are constant; [container.reqmts]/50,65: t.swap(s) is constant,
// exchanges the values "without invoking any move, copy, or swap operations on the individual
// container elements", and iterators keep referring to the same elements, now in the other
// container. Constant complexity independent of size means the elements are not copied or
// moved: they stay at the same addresses and are now owned by the destination. Move
// assignment with std::allocator (propagate_on_container_move_assignment is true,
// [default.allocator]) likewise takes over the elements.
// make_with(n) builds a container of n elements; the container need not provide size().
#pragma once
#include <memory>
#include <utility>

namespace reqs::move_swap_stability {

template <class X>
constexpr bool same_addresses(const X& x, const void* const* addr, int n) {
  int k = 0;
  for (auto it = x.begin(); it != x.end(); ++it, ++k)
    if (k >= n || static_cast<const void*>(std::addressof(*it)) != addr[k]) return false;
  return k == n;
}

template <class X, class Make>
constexpr bool test(Make make_with) {
  constexpr int N = 60;
  const void* addr[N];
  X v = make_with(N);
  {
    int k = 0;
    for (auto it = v.begin(); it != v.end(); ++it) addr[k++] = std::addressof(*it);
    if (k != N) return false;
  }
  X u(std::move(v));
  if (!same_addresses(u, addr, N)) return false;
  const auto al = u.get_allocator();
  X w(std::move(u), al);
  if (!same_addresses(w, addr, N)) return false;
  X x = make_with(3);
  x = std::move(w);
  if (!same_addresses(x, addr, N)) return false;
  X y = make_with(2);
  auto yfirst = y.begin();
  const void* yaddr[2];
  {
    int k = 0;
    for (auto it = y.begin(); it != y.end(); ++it) yaddr[k++] = std::addressof(*it);
  }
  auto xfirst = x.begin();
  x.swap(y);
  if (!same_addresses(y, addr, N) || !same_addresses(x, yaddr, 2)) return false;
  if (xfirst != y.begin() || yfirst != x.begin()) return false;
  using std::swap;
  swap(x, y);
  if (!same_addresses(x, addr, N) || !same_addresses(y, yaddr, 2)) return false;
  return true;
}

}  // namespace reqs::move_swap_stability
