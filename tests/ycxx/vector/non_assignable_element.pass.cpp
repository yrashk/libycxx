// Operations that only append or remove at the end, or reallocate, need T to be
// Cpp17MoveInsertable / Cpp17CopyInsertable but not assignable: push_back / emplace_back
// ([sequence.reqmts]/85,102,106), pop_back, reserve and shrink_to_fit ([vector.capacity]
// /3,8), resize ([vector.capacity]/14,17), clear, copy and move construction, swap and
// move assignment with std::allocator (pointer exchange; [container.reqmts]/65 forbids
// element-wise operations for swap). Those must work for a T whose assignment operators are
// deleted. (insert/erase in the middle require Cpp17MoveAssignable, [sequence.reqmts]/29,46,
// and are not used.)
#include <vector>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct NoAssign {
  int v;
  constexpr NoAssign(int x = 0) : v(x) {}
  constexpr NoAssign(const NoAssign&) = default;
  constexpr NoAssign(NoAssign&&) noexcept = default;
  NoAssign& operator=(const NoAssign&) = delete;
  NoAssign& operator=(NoAssign&&) = delete;
};
static_assert(!std::is_copy_assignable_v<NoAssign> && !std::is_move_assignable_v<NoAssign>);

constexpr bool test() {
  std::vector<NoAssign> v;
  for (int i = 0; i < 20; ++i) v.emplace_back(i);
  NoAssign x(20);
  v.push_back(x);
  v.push_back(NoAssign(21));
  v.reserve(100);
  v.shrink_to_fit();
  v.pop_back();
  v.resize(25);
  v.resize(30, NoAssign(7));
  if (v.size() != 30 || v[20].v != 20 || v[21].v != 0 || v[29].v != 7) return false;
  v.resize(10);
  std::vector<NoAssign> c(v);
  std::vector<NoAssign> m(std::move(c));
  std::vector<NoAssign> w(3, NoAssign(1));
  w.swap(m);
  if (w.size() != 10 || w[9].v != 9 || m.size() != 3) return false;
  m = std::move(w);  // std::allocator propagates on move assignment: no element assignment
  if (m.size() != 10 || m[5].v != 5) return false;
  m.clear();
  return m.empty();
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
