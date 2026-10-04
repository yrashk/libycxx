// [range.cache.latest]: cache_latest_view is an input-only view whose operator* evaluates
// the underlying element once per position ([range.cache.latest.iterator]/6) and returns an
// lvalue reference to the cached value; ++ resets the cache (/4); not common, not
// const-iterable; sized as its base.
#include <iterator>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

struct Expensive {
  int* calls;
  constexpr int operator()(int x) const {
    ++*calls;
    return x * 10;
  }
};
using V = decltype(vw::iota(0, 4) | vw::transform(Expensive{nullptr}));
using CL = rg::cache_latest_view<V>;
static_assert(std::is_same_v<decltype(std::declval<V>() | vw::cache_latest), CL>);
static_assert(rg::input_range<CL> && !rg::forward_range<CL> && !rg::common_range<CL>);
static_assert(!rg::range<const CL> && rg::sized_range<CL> && !rg::borrowed_range<CL>);
static_assert(std::is_same_v<rg::range_reference_t<CL>, int&>);
static_assert(std::is_same_v<rg::range_rvalue_reference_t<CL>, int>);
static_assert(std::is_same_v<rg::iterator_t<CL>::iterator_concept, std::input_iterator_tag>);
static_assert(!std::copyable<rg::iterator_t<CL>>);
using CR = rg::cache_latest_view<rg::ref_view<int[2]>>;
static_assert(std::is_same_v<rg::range_reference_t<CR>, int&>);

constexpr bool test() {
  int calls = 0;
  auto c = vw::iota(0, 4) | vw::transform(Expensive{&calls}) | vw::cache_latest;
  CHECK(c.size() == 4u);
  auto it = c.begin();
  CHECK(*it == 0 && *it == 0 && calls == 1);
  ++it;
  CHECK(*it == 10 && *it == 10 && calls == 2);
  int& r = *it;
  r = 99;
  CHECK(*it == 99 && calls == 2); // the cached value is returned by reference
  int sum = 0;
  for (; it != c.end(); ++it) sum += *it;
  CHECK(sum == 99 + 20 + 30 && calls == 4);
  // Filter after transform evaluates the transform twice per element without cache_latest.
  int calls2 = 0;
  auto f = vw::iota(0, 4) | vw::transform(Expensive{&calls2}) | vw::cache_latest |
           vw::filter([](int x) { return x > 10; });
  int n = 0;
  for (int x : f) n += x;
  CHECK(n == 50 && calls2 == 4);
  int a[2] = {1, 2};
  auto cr = a | vw::cache_latest;
  *cr.begin() = 5;
  CHECK(a[0] == 5);
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
