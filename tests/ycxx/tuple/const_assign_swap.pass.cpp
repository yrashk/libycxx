// [tuple.assign]: the const-qualified assignment operators ("constexpr const tuple&
// operator=(...) const") assign through a const tuple whose elements are references
// (constraints is_assignable_v<const Ti&, ...>). [tuple.swap], [tuple.special]: the const
// overloads of swap.
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

using Refs = std::tuple<int&, long&>;

// Constraint checks.
static_assert(std::is_assignable_v<const Refs&, const std::tuple<int, long>&>);
static_assert(std::is_assignable_v<const Refs&, std::tuple<int, long>&&>);
static_assert(std::is_assignable_v<const Refs&, const std::pair<short, int>&>);
static_assert(std::is_assignable_v<const Refs&, std::pair<short, int>&&>);
static_assert(std::is_assignable_v<const Refs&, const Refs&>);
static_assert(std::is_assignable_v<const Refs&, Refs&&>);
static_assert(!std::is_assignable_v<const std::tuple<int>&, const std::tuple<int>&>);
static_assert(!std::is_assignable_v<const std::tuple<int, long>&, const std::pair<int, long>&>);
static_assert(!std::is_assignable_v<const std::tuple<const int&>&, const std::tuple<int>&>);
static_assert(!std::is_assignable_v<const Refs&, const std::tuple<int>&>);  // size mismatch
static_assert(std::is_same_v<decltype(std::declval<const Refs&>() = std::declval<const std::tuple<int, long>&>()),
                             const Refs&>);
static_assert(std::is_same_v<decltype(std::declval<const Refs&>() = std::declval<std::pair<int, long>>()),
                             const Refs&>);
static_assert(std::is_swappable_v<const Refs>);
static_assert(!std::is_swappable_v<const std::tuple<int>>);

// An element type with a const-qualified assignment that records the value category.
struct Proxy {
  int* target;
  int* how;
  constexpr const Proxy& operator=(const int& v) const { *target = v; *how = 1; return *this; }
  constexpr const Proxy& operator=(int&& v) const { *target = v; *how = 2; return *this; }
};

constexpr bool test() {
  int a = 0;
  long b = 0;
  const Refs t(a, b);
  {
    const std::tuple<int, long> src(1, 2);
    auto& r = (t = src);
    if (&r != &t || a != 1 || b != 2) return false;
  }
  t = std::tuple<int, long>(3, 4);
  if (a != 3 || b != 4) return false;
  {
    const std::pair<short, int> p(5, 6);
    t = p;
    if (a != 5 || b != 6) return false;
  }
  t = std::pair<short, int>(7, 8);
  if (a != 7 || b != 8) return false;
  {
    int c = 9;
    long d = 10;
    const Refs u(c, d);
    t = u;                                   // const tuple& operator=(const tuple&) const
    if (a != 9 || b != 10) return false;
    if (&std::get<0>(t) != &a) return false; // references are not rebound
    c = 11;
    t = std::move(u);                        // const tuple& operator=(tuple&&) const
    if (a != 11) return false;
  }
  {
    // Value category is forwarded: tuple&& source assigns from rvalues.
    int x = 0, how = 0;
    const std::tuple<Proxy> pt(Proxy{&x, &how});
    std::tuple<int> s(42);
    pt = s;
    if (x != 42 || how != 1) return false;
    pt = std::tuple<int>(43);
    if (x != 43 || how != 2) return false;
    std::pair<int, int> pp(44, 0);
    const std::tuple<Proxy, Proxy> pt2(Proxy{&x, &how}, Proxy{&x, &how});
    pt2 = std::move(pp);
    if (x != 0 || how != 2) return false;     // second element assigned last
  }
  {
    // swap of const tuples of references swaps the referents.
    int p = 1, q = 2;
    long r = 3, s = 4;
    const Refs t1(p, r), t2(q, s);
    t1.swap(t2);
    if (p != 2 || q != 1 || r != 4 || s != 3) return false;
    swap(t1, t2);
    if (p != 1 || q != 2 || r != 3 || s != 4) return false;
    std::swap(t1, t2);
    if (p != 2 || q != 1) return false;
  }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
