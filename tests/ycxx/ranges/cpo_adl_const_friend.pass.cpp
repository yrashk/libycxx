// The range access customization point objects find a free function by "argument-dependent
// lookup only" -- there is no poison-pill declaration in the current draft:
// [range.access.begin]/2.5, [range.access.end]/2.6, [range.access.rbegin]/2.4,
// [range.access.rend]/2.4, [range.prim.size]/2.4 (and [range.prim.size.hint]/2.3, checked in
// ranges/reserve_hint.pass.cpp): "if T is a
// class or enumeration type and auto(begin(t)) is a valid expression whose type models
// input_or_output_iterator where the meaning of begin is established as-if by performing
// argument-dependent lookup only ([basic.lookup.argdep]), then ranges::begin(E) is
// expression-equivalent to that expression." The most common way to write such a function is
// a hidden friend taking const T&; ADL finds it for a non-const lvalue t, so it must be used.
// (A deleted unconstrained "begin(auto&)" in the lookup set, as in C++20's wording, would be
// a better match than the const& friend for a non-const t and make the call ill-formed.)
// Dependent CPOs follow: cbegin / cend ([range.access.cbegin]), data via begin for a
// contiguous iterator ([range.prim.data]/2.3), ssize ([range.prim.ssize]), empty
// ([range.prim.empty]/2.3: ranges::begin(t) == ranges::end(t)), and the range / sized_range /
// contiguous_range concepts.
#include <ranges>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include "check.hpp"

namespace user {
struct R {  // begin / end only as const& hidden friends
  int a[3] = {1, 2, 3};
  friend constexpr const int* begin(const R& r) { return r.a; }
  friend constexpr const int* end(const R& r) { return r.a + 3; }
};

struct Rev {  // also rbegin / rend as const& hidden friends
  int a[3] = {1, 2, 3};
  friend constexpr const int* begin(const Rev& r) { return r.a; }
  friend constexpr const int* end(const Rev& r) { return r.a + 3; }
  friend constexpr std::reverse_iterator<const int*> rbegin(const Rev& r) {
    return std::reverse_iterator<const int*>(r.a + 2);  // deliberately not the default (skips 3)
  }
  friend constexpr std::reverse_iterator<const int*> rend(const Rev& r) { return std::reverse_iterator<const int*>(r.a); }
};

struct Sized {  // member begin / end, size as a const& hidden friend
  int a[4] = {};
  constexpr const int* begin() const { return a; }
  constexpr const int* end() const { return a + 4; }
  friend constexpr std::size_t size(const Sized&) { return 99; }  // deliberately not end - begin
};

}  // namespace user

static_assert(std::ranges::contiguous_range<user::R> && std::ranges::contiguous_range<const user::R>);
static_assert(std::ranges::sized_range<user::R>);  // via end - begin
static_assert(std::ranges::bidirectional_range<user::Rev>);
static_assert(std::ranges::sized_range<user::Sized>);

constexpr bool test() {
  user::R r;
  const user::R& cr = r;
  if (std::ranges::begin(r) != r.a || std::ranges::end(r) != r.a + 3) return false;
  if (std::ranges::begin(cr) != r.a || std::ranges::end(cr) != r.a + 3) return false;
  if (std::ranges::cbegin(r) != r.a || std::ranges::cend(r) != r.a + 3) return false;
  if (std::ranges::data(r) != r.a || std::ranges::size(r) != 3 || std::ranges::ssize(r) != 3) return false;
  if (std::ranges::empty(r)) return false;
  if (*std::ranges::rbegin(r) != 3) return false;  // default: reverse_iterator(end)
  int sum = 0;
  for (int x : r.a) sum += x;
  int sum2 = 0;
  for (auto it = std::ranges::begin(r); it != std::ranges::end(r); ++it) sum2 += *it;
  if (sum != sum2) return false;

  user::Rev v;
  if (*std::ranges::rbegin(v) != 2) return false;  // the friend, not reverse_iterator(end)
  if (std::ranges::rend(v) != std::reverse_iterator<const int*>(v.a)) return false;
  if (*std::ranges::crbegin(v) != 2) return false;

  user::Sized s;
  if (std::ranges::size(s) != 99 || std::ranges::ssize(s) != 99) return false;
  if (std::ranges::empty(s)) return false;  // via size() == 0

  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
