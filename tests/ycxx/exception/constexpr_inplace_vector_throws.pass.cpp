// inplace_vector reports errors during constant evaluation as at run time (P3068; every member
// is constexpr):
// [inplace.vector.overview]/4: a member that would make the size exceed N throws bad_alloc;
// [inplace.vector.capacity]/9: reserve(n) throws bad_alloc if n > capacity(); /4, /7: resize
//   has no effects if it throws;
// [inplace.vector.modifiers]/5, /7: push_back / emplace_back throw bad_alloc (or what the
//   element's initialization throws) and then have no effects on *this; /12: try_push_back
//   throws nothing unless the initialization does, and returns an empty optional when full;
//   /3: insert has no effects when it throws other than from T's copy/move operations;
// [sequence.reqmts]/127: at(n) throws out_of_range if n >= size().
// Both for int and for a literal class type whose copy constructor can throw.
// XFAIL: clang Clang 23 cannot throw during constant evaluation (P3068's core-language part)
// REQUIRES: exceptions
#include <inplace_vector>
#include <new>
#include <stdexcept>
#include <utility>
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

constexpr bool ints() {
  std::inplace_vector<int, 3> v{1, 2, 3};
  int ok = 0;
  ok += throws<std::bad_alloc>([&] { v.push_back(4); });
  ok += throws<std::bad_alloc>([&] { v.emplace_back(4); });
  ok += throws<std::bad_alloc>([&] { v.insert(v.begin(), 0); });
  ok += throws<std::bad_alloc>([&] { v.insert(v.end(), 2, 9); });
  ok += throws<std::bad_alloc>([&] { v.resize(4); });
  ok += throws<std::bad_alloc>([&] { v.resize(5, 7); });
  ok += throws<std::bad_alloc>([&] { v.reserve(4); });
  ok += throws<std::bad_alloc>([&] { v.append_range(std::inplace_vector<int, 1>{5}); });
  ok += throws<std::bad_alloc>([&] { std::inplace_vector<int, 3> w(4); });
  ok += throws<std::bad_alloc>([&] { std::inplace_vector<int, 3> w(4, 1); });
  ok += throws<std::bad_alloc>([&] { std::inplace_vector<int, 3> w{1, 2, 3, 4}; });
  ok += throws<std::bad_alloc>([&] { v.assign(4, 0); });
  ok += throws<std::out_of_range>([&] { (void)v.at(3); });
  ok += throws<std::out_of_range>([&] { (void)std::as_const(v).at(3); });
  ok += !throws<std::bad_alloc>([&] { v.reserve(3); });
  ok += !v.try_push_back(4).has_value();
  ok += !v.try_emplace_back(4).has_value();
  return ok == 17 && v.size() == 3 && v[0] == 1 && v[1] == 2 && v[2] == 3;
}
static_assert(ints());

// Zero capacity: every insertion throws.
constexpr bool zero() {
  std::inplace_vector<int, 0> v;
  return throws<std::bad_alloc>([&] { v.push_back(1); }) && throws<std::out_of_range>([&] { (void)v.at(0); }) &&
         v.empty();
}
static_assert(zero());

// A literal type whose copy throws once the budget runs out.
struct item {
  int v;
  int* budget;
  constexpr item(int x, int* b) : v(x), budget(b) {}
  constexpr item(const item& o) : v(o.v), budget(o.budget) {
    if (*budget == 0) throw std::runtime_error("copy");
    --*budget;
  }
  constexpr item& operator=(const item&) = default;
  constexpr ~item() {}
};

constexpr bool throwing_copies() {
  int budget = 100;
  std::inplace_vector<item, 4> v;
  v.emplace_back(1, &budget);
  v.emplace_back(2, &budget);
  const item extra(3, &budget);
  int ok = 0;
  budget = 0;
  // push_back: the copy throws; no effects
  ok += throws<std::runtime_error>([&] { v.push_back(extra); });
  ok += v.size() == 2 && v[0].v == 1 && v[1].v == 2;
  // try_push_back: the initialization's exception propagates; no effects
  ok += throws<std::runtime_error>([&] { (void)v.try_push_back(extra); });
  ok += v.size() == 2;
  // resize(n, c): no effects
  ok += throws<std::runtime_error>([&] { v.resize(4, extra); });
  ok += v.size() == 2 && v[1].v == 2;
  // full: bad_alloc before any copy is attempted
  budget = 2;
  v.push_back(extra);
  v.push_back(extra);
  ok += budget == 0;
  budget = 5;
  ok += throws<std::bad_alloc>([&] { v.push_back(extra); });
  ok += budget == 5 && v.size() == 4;
  return ok == 9;
}
static_assert(throwing_copies());

int main() {
  CHECK(ints());
  CHECK(zero());
  CHECK(throwing_copies());
}
