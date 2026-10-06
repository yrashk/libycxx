// list, forward_list and deque keep their exception guarantees during constant evaluation (P3068;
// the three are constexpr in C++26, P3372):
// [list.modifiers]/2: if an exception is thrown by insert, emplace, push_front, push_back,
//   emplace_front, emplace_back, prepend_range or append_range, there are no effects;
// [forward.list.modifiers]/1: if an exception is thrown by any of the modifiers, there is no
//   effect on the container;
// [deque.modifiers]/3: if an exception is thrown while inserting a single element at either end,
//   there are no effects;
// [container.reqmts]/66.1-66.2 for insert / emplace of a single element.
// The exception comes from the element's constructor, after the container may have allocated.
// XFAIL: clang Clang 23 cannot throw during constant evaluation (P3068's core-language part)
// REQUIRES: exceptions
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <stdexcept>
#include "check.hpp"

struct elem {
  int v;
  constexpr elem(int x) : v(x) {
    if (x < 0) throw std::invalid_argument("negative");
  }
  constexpr bool operator==(const elem&) const = default;
};

template <class F>
constexpr bool throws(F f) {
  try {
    f();
  } catch (const std::invalid_argument&) {
    return true;
  } catch (...) {
    return false;
  }
  return false;
}

template <class C>
constexpr bool same(const C& c, std::initializer_list<int> want) {
  auto it = c.begin();
  for (int w : want) {
    if (it == c.end() || it->v != w) return false;
    ++it;
  }
  return it == c.end();
}

constexpr bool lists() {
  std::list<elem> l{1, 2, 3};
  const auto first = l.begin();
  int ok = 0;
  ok += throws([&] { l.emplace_back(-1); });
  ok += throws([&] { l.emplace_front(-1); });
  ok += throws([&] { l.emplace(std::next(l.begin()), -1); });
  ok += throws([&] { l.insert(l.end(), {4, -1, 5}); }); // no effects, not even 4
  ok += throws([&] { l.append_range(std::list<int>{6, -6}); });
  ok += throws([&] { l.prepend_range(std::list<int>{-7}); });
  ok += same(l, {1, 2, 3}) && l.size() == 3 && l.begin() == first;
  std::forward_list<elem> f{1, 2};
  ok += throws([&] { f.emplace_front(-1); });
  ok += throws([&] { f.emplace_after(f.begin(), -1); });
  ok += throws([&] { f.insert_after(f.begin(), {7, -1}); });
  ok += throws([&] { f.prepend_range(std::list<int>{8, -8}); });
  ok += same(f, {1, 2});
  return ok == 12;
}
static_assert(lists());

constexpr bool deques() {
  std::deque<elem> d;
  for (int i = 0; i < 40; ++i) d.emplace_back(i); // more than one block
  int ok = 0;
  for (int k = 0; k < 3; ++k) {
    ok += throws([&] { d.emplace_back(-1); });
    ok += throws([&] { d.emplace_front(-1); });
    ok += d.size() == 40 && d.front().v == 0 && d.back().v == 39;
    d.emplace_back(40 + k); // still usable
    d.pop_back();
  }
  for (int i = 0; i < 40; ++i)
    ok += d[i].v == i;
  return ok == 49;
}
static_assert(deques());

int main() {
  CHECK(lists());
  CHECK(deques());
}
