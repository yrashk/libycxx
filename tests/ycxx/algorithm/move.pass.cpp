// [alg.move]: move(first, last, result) move-assigns *(result + n) = std::move(*(first + n))
// in order and returns result + N; move_backward moves into [result - N, result) from last - 1
// and returns result - N. ranges::move / ranges::move_backward use ranges::iter_move and
// return {last, result + N} / {last, result - N}. Only move assignments are used.
#include <algorithm>
#include <ranges>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct M {
  int v = 0;
  bool moved_from = false;
  int copies = 0;
  constexpr M() = default;
  constexpr explicit M(int x) : v(x) {}
  constexpr M(const M& o) : v(o.v), copies(o.copies + 1) {}
  constexpr M(M&& o) noexcept : v(o.v) { o.moved_from = true; }
  constexpr M& operator=(const M& o) {
    v = o.v;
    copies = o.copies + 1;
    return *this;
  }
  constexpr M& operator=(M&& o) noexcept {
    v = o.v;
    copies = 0;
    o.moved_from = true;
    return *this;
  }
};

constexpr bool test() {
  M src[3] = {M(1), M(2), M(3)};
  M dst[3];
  M* r = std::move(src, src + 3, dst);
  if (r != dst + 3) return false;
  for (int i = 0; i < 3; ++i)
    if (dst[i].v != i + 1 || dst[i].copies != 0 || !src[i].moved_from) return false;

  M s2[3] = {M(4), M(5), M(6)};
  M d2[4];
  M* rb = std::move_backward(s2, s2 + 3, d2 + 4);
  if (rb != d2 + 1 || d2[1].v != 4 || d2[3].v != 6 || !s2[0].moved_from) return false;

  // overlapping left / right
  int a[] = {1, 2, 3, 4, 5};
  std::move(a + 2, a + 5, a);
  if (a[0] != 3 || a[2] != 5) return false;
  int b[] = {1, 2, 3, 4, 5};
  std::move_backward(b, b + 3, b + 5);
  if (b[2] != 1 || b[4] != 3) return false;

  M s3[2] = {M(7), M(8)};
  M d3[2];
  auto res = std::ranges::move(s3, d3);
  static_assert(std::is_same_v<decltype(res), std::ranges::move_result<M*, M*>>);
  if (res.in != s3 + 2 || res.out != d3 + 2 || d3[1].v != 8 || d3[1].copies != 0 || !s3[1].moved_from)
    return false;

  M s4[2] = {M(9), M(10)};
  M d4[3];
  auto resb = std::ranges::move_backward(s4, d4 + 3);
  static_assert(std::is_same_v<decltype(resb), std::ranges::move_backward_result<M*, M*>>);
  if (resb.in != s4 + 2 || resb.out != d4 + 1 || d4[2].v != 10 || !s4[0].moved_from) return false;
  return true;
}

static_assert(test());

// ranges::move uses iter_move: a custom iter_move customization is honoured
struct Tracker {
  static inline int custom_iter_moves = 0;
};
struct It {
  using value_type = int;
  using difference_type = long;
  int* p;
  int& operator*() const { return *p; }
  It& operator++() {
    ++p;
    return *this;
  }
  It operator++(int) {
    It t = *this;
    ++p;
    return t;
  }
  bool operator==(const It&) const = default;
  friend int&& iter_move(const It& i) {
    ++Tracker::custom_iter_moves;
    return std::move(*i.p);
  }
};

int main() {
  CHECK(test());
  int s[3] = {1, 2, 3}, d[3] = {};
  std::ranges::move(It{s}, It{s + 3}, d);
  CHECK(Tracker::custom_iter_moves == 3);
  CHECK(d[2] == 3);
  return 0;
}
