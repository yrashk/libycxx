// The container adaptors' default constructors are defined in their synopses:
// [queue.defn] "queue() : queue(Container()) {}", [stack.defn] "stack() : stack(Container())
// {}", [priqueue.overview] "priority_queue() : priority_queue(Compare()) {}" and "explicit
// priority_queue(const Compare& x) : priority_queue(x, Container()) {}". The prvalue
// Container() binds to the Container&& overload, which "Initializes c with std::move(cont)"
// ([queue.cons]/2, [stack.cons]/2; [priqueue.cons]/2 "move constructing"). So one default
// construction of Container and one move construction, no copy; a container type with a copy
// constructor but no move constructor is copied instead (overload resolution of
// c(std::move(cont)) picks the copy constructor).
// COUNTERPART: libstdcxx:23_containers/(priority_queue|queue|stack)/77528.cc
#include <deque>
#include <functional>
#include <queue>
#include <stack>
#include <utility>
#include <vector>
#include "check.hpp"

struct counts {
  int defaults = 0, copies = 0, moves = 0;
};
counts n;

template <class Base>
struct counting : Base {
  counting() { ++n.defaults; }
  counting(const counting& o) : Base(o) { ++n.copies; }
  counting(counting&& o) noexcept : Base(std::move(o)) { ++n.moves; }
  counting& operator=(const counting&) = default;
  counting& operator=(counting&&) = default;
};

template <class Base>
struct copy_only : Base {  // the move constructor is not declared: rvalues copy
  copy_only() { ++n.defaults; }
  copy_only(const copy_only& o) : Base(o) { ++n.copies; }
  copy_only& operator=(const copy_only&) = default;
};

template <class Adaptor>
counts make() {
  n = counts{};
  Adaptor a;
  CHECK(a.empty() && a.size() == 0);
  return n;
}

int main() {
  using DQ = counting<std::deque<int>>;
  using VC = counting<std::vector<int>>;
  counts c = make<std::queue<int, DQ>>();
  CHECK(c.defaults == 1 && c.moves == 1 && c.copies == 0);
  c = make<std::stack<int, VC>>();
  CHECK(c.defaults == 1 && c.moves == 1 && c.copies == 0);
  c = make<std::priority_queue<int, VC>>();
  CHECK(c.defaults == 1 && c.moves == 1 && c.copies == 0);
  c = make<std::priority_queue<int, VC, std::greater<int>>>();
  CHECK(c.defaults == 1 && c.moves == 1 && c.copies == 0);

  using CQ = copy_only<std::deque<int>>;
  using CV = copy_only<std::vector<int>>;
  c = make<std::queue<int, CQ>>();
  CHECK(c.defaults == 1 && c.copies == 1);
  c = make<std::stack<int, CV>>();
  CHECK(c.defaults == 1 && c.copies == 1);
  c = make<std::priority_queue<int, CV>>();
  CHECK(c.defaults == 1 && c.copies == 1);

  // The adaptors work normally afterwards.
  std::priority_queue<int, VC> p;
  p.push(3);
  p.push(9);
  p.push(1);
  CHECK(p.top() == 9);
}
