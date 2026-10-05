// [vector.cons]: vector(const Allocator&) (empty, noexcept); explicit vector(n, a) with n
// default-inserted (value-initialized) elements; vector(n, value, a); vector(first, last,
// a) equal to the range; vector(from_range, rg, a); [vector.overview] copy/move
// constructors, allocator-extended copy/move (type_identity_t<Allocator>), initializer_list.
// [sequence.reqmts]/9,12: each iterator in the range is dereferenced exactly once.
// [sequence.reqmts]/69.1: integral arguments do not select the iterator-pair constructor.
// REQUIRES: exceptions
#include <vector>
#include <list>
#include <ranges>
#include <type_traits>
#include <utility>
#include "test_allocators.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

static_assert(std::is_nothrow_default_constructible_v<std::vector<int>>);
static_assert(std::is_nothrow_constructible_v<std::vector<int>, const std::allocator<int>&>);
static_assert(std::is_nothrow_move_constructible_v<std::vector<int>>);
static_assert(!std::is_convertible_v<std::size_t, std::vector<int>>);  // explicit (n)
static_assert(!std::is_convertible_v<const std::allocator<int>&, std::vector<int>>);
static_assert(std::is_constructible_v<std::vector<int>, int, int>);  // (n, value)

constexpr bool test() {
  {
    std::vector<int> v;
    if (!v.empty() || v.size() != 0 || v.begin() != v.end()) return false;
  }
  {
    std::vector<int> v(5);
    if (v.size() != 5) return false;
    for (int x : v)
      if (x != 0) return false;  // default-inserted ints are value-initialized
  }
  {
    std::vector<int> v(3, 7);
    if (v.size() != 3 || v[0] != 7 || v[2] != 7) return false;
    std::vector<long> w(4, 2);  // integral pair: still (n, value)
    if (w.size() != 4 || w[3] != 2) return false;
  }
  {
    int a[] = {1, 2, 3, 4};
    int derefs = 0;
    std::vector<int> v(InputIter<int>(a, &derefs), InputIter<int>(a + 4, &derefs));
    if (v.size() != 4 || v[3] != 4 || derefs != 4) return false;
    derefs = 0;
    std::vector<int> f(ForwardIter<int>(a, &derefs), ForwardIter<int>(a + 3, &derefs));
    if (f.size() != 3 || f[2] != 3 || derefs != 3) return false;
    std::vector<long> conv(a, a + 2);  // element type converts
    if (conv.size() != 2 || conv[1] != 2L) return false;
  }
  {
    int a[] = {5, 6, 7};
    int derefs = 0;
    std::vector<int> v(std::from_range, InputRange<int>{a, a + 3, &derefs});
    if (v.size() != 3 || v[0] != 5 || derefs != 3) return false;
    derefs = 0;
    std::vector<int> w(std::from_range, ForwardRange<int>{a, a + 2, &derefs});
    if (w.size() != 2 || derefs != 2) return false;
    std::vector<int> x(std::from_range, std::views::iota(0, 10));
    if (x.size() != 10 || x[9] != 9) return false;
    std::vector<double> y(std::from_range, a);
    if (y.size() != 3 || y[2] != 7.0) return false;
  }
  {
    std::vector<int> v{1, 2, 3};
    if (v.size() != 3 || v[1] != 2) return false;
    std::vector<int> c(v);
    if (c != v || c.data() == v.data()) return false;
    std::vector<int> m(std::move(c));
    if (m != v) return false;
    std::vector<int> e{};
    if (!e.empty()) return false;
    std::vector<int> two{3, 4};  // initializer_list, not (n, value)
    if (two.size() != 2 || two[0] != 3) return false;
    std::vector<int> paren(3, 4);
    if (paren.size() != 3) return false;
  }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  {
    std::list<int> l = {1, 2, 3};
    std::vector<int> v(l.begin(), l.end());
    CHECK(v.size() == 3 && v[2] == 3);
  }
  {
    using A = IdAlloc<int>;
    std::vector<int, A> a(A(1));
    CHECK(a.empty() && a.get_allocator().id == 1);
    std::vector<int, A> b(3, A(2));
    CHECK(b.size() == 3 && b[2] == 0 && b.get_allocator().id == 2);
    std::vector<int, A> c(3, 9, A(3));
    CHECK(c.size() == 3 && c[0] == 9 && c.get_allocator().id == 3);
    int arr[] = {1, 2};
    std::vector<int, A> d(arr, arr + 2, A(4));
    CHECK(d.size() == 2 && d.get_allocator().id == 4);
    std::vector<int, A> e(std::from_range, arr, A(5));
    CHECK(e.size() == 2 && e.get_allocator().id == 5);
    std::vector<int, A> f({7, 8}, A(6));
    CHECK(f.size() == 2 && f.get_allocator().id == 6);
    std::vector<int, A> g(f, A(7));
    CHECK(g == f && g.get_allocator().id == 7);
    std::vector<int, A> h(std::move(g), A(8));  // unequal allocator
    CHECK(h == f && h.get_allocator().id == 8);
    std::vector<int, A> i(std::move(h), A(8));  // equal allocator
    CHECK(i == f && i.get_allocator().id == 8);
    std::vector<int, A> j(i);
    CHECK(j.get_allocator().id == 8);  // select_on_container_copy_construction default
    std::vector<int, A> k(std::move(j));
    CHECK(k.get_allocator().id == 8 && k == f);
    // The allocator parameter of the extended constructors is a non-deduced context:
    // braces work.
    std::vector<int, A> l(f, {});
    CHECK(l == f && l.get_allocator().id == 0);
  }
  return 0;
}
