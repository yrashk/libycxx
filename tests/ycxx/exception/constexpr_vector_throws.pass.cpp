// vector reports errors and keeps its exception guarantees during constant evaluation as at run
// time (P3068; vector's members are constexpr):
// [vector.capacity]/4: if an exception is
//   thrown (here: by the allocator) other than by the move constructor of a
//   non-Cpp17CopyInsertable type, there are no effects; /16, /19: the same for resize;
// [vector.modifiers]/2: an exception while inserting a single element at the end of a vector of
//   a Cpp17CopyInsertable T has no effects, also when reallocating and T's move constructor may
//   throw (the old elements must then be copied, not moved); an exception from the allocator has
//   no effects for every insertion.
// XFAIL: clang Clang 23 cannot throw during constant evaluation (P3068's core-language part)
// REQUIRES: exceptions
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>
#include "check.hpp"

template <class E, class F>
constexpr bool throws(F f) {
  try {
    f();
  } catch (const E&) {
    return true;
  } catch (...) {
    return false;
  }
  return false;
}

// An allocator that throws bad_alloc once *budget allocations have been made.
template <class T>
struct budget_alloc {
  using value_type = T;
  int* budget;
  constexpr explicit budget_alloc(int* b) : budget(b) {}
  template <class U>
  constexpr budget_alloc(const budget_alloc<U>& o) : budget(o.budget) {}
  constexpr T* allocate(std::size_t n) {
    if (*budget == 0) throw std::bad_alloc();
    --*budget;
    return std::allocator<T>().allocate(n);
  }
  constexpr void deallocate(T* p, std::size_t n) { std::allocator<T>().deallocate(p, n); }
  template <class U>
  constexpr bool operator==(const budget_alloc<U>& o) const { return budget == o.budget; }
};

constexpr bool allocator_failures() {
  int budget = 1;
  std::vector<int, budget_alloc<int>> v{budget_alloc<int>(&budget)};
  v.reserve(4); // the one allocation allowed
  v.assign({1, 2, 3, 4});
  const int* data = v.data();
  int ok = 0;
  ok += throws<std::bad_alloc>([&] { v.push_back(5); });
  ok += throws<std::bad_alloc>([&] { v.emplace_back(5); });
  ok += throws<std::bad_alloc>([&] { v.insert(v.begin(), 0); });
  ok += throws<std::bad_alloc>([&] { v.insert(v.begin() + 2, {7, 8}); });
  ok += throws<std::bad_alloc>([&] { v.resize(6); });
  ok += throws<std::bad_alloc>([&] { v.resize(6, 9); });
  ok += throws<std::bad_alloc>([&] { v.reserve(5); });
  ok += throws<std::bad_alloc>([&] { v.append_range(std::vector<int>{5, 6}); });
  // no effects: same elements, same storage, same capacity
  ok += v.size() == 4 && v.capacity() == 4 && v.data() == data;
  ok += v[0] == 1 && v[1] == 2 && v[2] == 3 && v[3] == 4;
  return ok == 10;
}
static_assert(allocator_failures());

// Copy throws when the budget runs out; the move constructor may throw (not noexcept) and
// leaves its source zeroed, so moving the old elements during reallocation would be visible.
struct item {
  int v;
  int* budget;
  constexpr item(int x, int* b) : v(x), budget(b) {}
  constexpr item(const item& o) : v(o.v), budget(o.budget) {
    if (*budget == 0) throw std::runtime_error("copy");
    --*budget;
  }
  constexpr item(item&& o) : v(o.v), budget(o.budget) { o.v = 0; }
  constexpr item& operator=(const item&) = default;
  constexpr item& operator=(item&&) = default;
  constexpr ~item() {}
};

constexpr bool throwing_copy_at_end() {
  int budget = 100;
  std::vector<item> v;
  v.reserve(3);
  v.emplace_back(1, &budget);
  v.emplace_back(2, &budget);
  v.emplace_back(3, &budget);
  const item extra(4, &budget);
  int ok = 0;
  // reallocation: the new element and the three old ones are copies; let the 3rd copy throw
  for (int b = 0; b < 4; ++b) {
    budget = b;
    ok += throws<std::runtime_error>([&] { v.push_back(extra); });
    ok += v.size() == 3 && v[0].v == 1 && v[1].v == 2 && v[2].v == 3;
  }
  // without reallocation
  budget = 100;
  v.reserve(10);
  budget = 0;
  ok += throws<std::runtime_error>([&] { v.push_back(extra); });
  ok += v.size() == 3 && v[2].v == 3;
  // reserve with a copy that throws: no effects
  ok += throws<std::runtime_error>([&] { v.reserve(20); });
  ok += v.size() == 3 && v.capacity() == 10 && v[0].v == 1;
  return ok == 12;
}
static_assert(throwing_copy_at_end());

int main() {
  CHECK(allocator_failures());
  CHECK(throwing_copy_at_end());
}
