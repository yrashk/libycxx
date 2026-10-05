// [move.iterator], [move.iter.elem], [move.iter.nav], [move.iter.op.conv],
// [move.iter.nonmember]: operator* is ranges::iter_move(current); operator[](n) is
// ranges::iter_move(current + n); base() const& returns const Iterator& (noexcept) and
// base() && returns std::move(current); operator++(int) returns void unless Iterator models
// forward_iterator; arithmetic and n + x; make_move_iterator.
// COUNTERPART: libcxx:iterators/predef.iterators/move.iterators/move.iter.ops/move.iter.op.comp/op_(gt|gte|lte).pass.cpp
#include <iterator>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Res {
  int v;
  bool moved_from = false;
  constexpr Res(int x) : v(x) {}
  constexpr Res(const Res& o) : v(o.v) {}
  constexpr Res(Res&& o) : v(o.v) { o.moved_from = true; }
  constexpr Res& operator=(const Res&) = default;
};

// input-only iterator (C++20 style, move-only)
struct InIt {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p;
  InIt(int* q) : p(q) {}
  InIt(InIt&&) = default;
  InIt& operator=(InIt&&) = default;
  int& operator*() const { return *p; }
  InIt& operator++() { ++p; return *this; }
  void operator++(int) { ++p; }
};
static_assert(std::input_iterator<InIt>);
static_assert(!std::forward_iterator<InIt>);

using M = std::move_iterator<int*>;
static_assert(std::is_same_v<decltype(*std::declval<M&>()), int&&>);
static_assert(std::is_same_v<decltype(std::declval<M&>()[0]), int&&>);
static_assert(std::is_same_v<decltype(std::declval<const M&>().base()), int* const&>);
static_assert(std::is_same_v<decltype(std::declval<M&&>().base()), int*>);
static_assert(noexcept(std::declval<const M&>().base()));
static_assert(std::is_same_v<decltype(std::declval<M&>()++), M>);
static_assert(std::is_same_v<decltype(std::declval<std::move_iterator<InIt>&>()++), void>);
static_assert(std::is_same_v<decltype(std::declval<M&>()--), M>);

constexpr bool test() {
  Res r[3] = {Res(1), Res(2), Res(3)};
  auto it = std::make_move_iterator(r + 0);
  static_assert(std::is_same_v<decltype(it), std::move_iterator<Res*>>);
  Res taken = *it;
  if (taken.v != 1 || !r[0].moved_from) return false;
  Res t2 = it[2];
  if (t2.v != 3 || !r[2].moved_from || r[1].moved_from) return false;
  Res& lv = *it.base();  // base gives access to the underlying iterator
  if (&lv != r) return false;
  auto it2 = it + 2;
  if (it2.base() != r + 2 || (2 + it).base() != r + 2 || (it2 - 1).base() != r + 1) return false;
  if (it2 - it != 2) return false;
  ++it;
  if (it.base() != r + 1) return false;
  auto old = it++;
  if (old.base() != r + 1 || it.base() != r + 2) return false;
  --it;
  it += 1;
  it -= 2;
  if (it.base() != r) return false;
  std::move_iterator<Res*> def;
  if (def.base() != nullptr) return false;
  std::move_iterator<const Res*> conv(it);  // converting constructor
  if (conv.base() != r) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  int a[3] = {1, 2, 3};
  std::move_iterator<InIt> mi{InIt(a)};
  mi++;
  CHECK(*mi.base() == 2);
  ++mi;
  CHECK(*mi == 3);
  InIt raw = std::move(mi).base();
  CHECK(raw.p == a + 2);
  return 0;
}
