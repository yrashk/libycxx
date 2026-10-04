// [inplace.vector.modifiers]/8-18: try_emplace_back(args...) / try_push_back(x) append an
// element when size() < capacity() and return optional<reference>(in_place, back());
// otherwise there are no effects (an rvalue argument is not moved from) and they return
// nullopt. unchecked_emplace_back / unchecked_push_back (precondition size() < capacity())
// are *try_emplace_back(...) / *try_push_back(...). push_back / emplace_back return back()
// (/4).
#include <inplace_vector>
#include <optional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct M {
  int v = 0;
  bool moved_from = false;
  constexpr M() = default;
  constexpr M(int x) : v(x) {}
  constexpr M(int a, int b) : v(a * 10 + b) {}
  constexpr M(const M&) = default;
  constexpr M(M&& o) : v(o.v) { o.moved_from = true; }
  constexpr M& operator=(const M&) = default;
  constexpr M& operator=(M&& o) {
    v = o.v;
    o.moved_from = true;
    return *this;
  }
};

constexpr bool test() {
  using IV = std::inplace_vector<M, 3>;
  IV v;
  static_assert(std::is_same_v<decltype(v.try_push_back(M())), std::optional<M&>>);
  static_assert(std::is_same_v<decltype(v.try_emplace_back(1, 2)), std::optional<M&>>);
  static_assert(std::is_same_v<decltype(v.unchecked_push_back(M())), M&>);
  static_assert(std::is_same_v<decltype(v.push_back(M())), M&>);
  static_assert(std::is_same_v<decltype(v.emplace_back()), M&>);
  auto r1 = v.try_emplace_back(1, 2);
  if (!r1 || &*r1 != &v.back() || r1->v != 12) return false;
  M m(5);
  auto r2 = v.try_push_back(m);
  if (!r2 || &*r2 != &v.back() || r2->v != 5 || v.size() != 2) return false;
  M& r3 = v.unchecked_emplace_back(7);
  if (&r3 != &v.back() || r3.v != 7 || v.size() != 3) return false;
  // full: no effects
  M n(9);
  auto f1 = v.try_push_back(std::move(n));
  auto f2 = v.try_push_back(m);
  auto f3 = v.try_emplace_back(4, 4);
  if (f1 || f2 || f3 || n.moved_from || v.size() != 3 || v.back().v != 7) return false;
  v.pop_back();
  M& r4 = v.unchecked_push_back(std::move(n));
  if (!n.moved_from || r4.v != 9 || &r4 != &v.back()) return false;
  v.pop_back();
  M& r5 = v.push_back(M(3));
  M& r6 = (v.pop_back(), v.emplace_back(2, 1));
  return &r6 == &v.back() && r6.v == 21 && r5.v == 21;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
