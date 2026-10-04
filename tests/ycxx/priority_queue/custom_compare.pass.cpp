// [priqueue.overview]: priority_queue keeps the protected members c and comp; [priqueue.cons]:
// the constructors initialize comp with the given comparator x (or value-initialize it) and
// call make_heap(c.begin(), c.end(), comp); [priqueue.members]: push / emplace call
// push_heap(c.begin(), c.end(), comp), pop calls pop_heap(c.begin(), c.end(), comp), and
// push_range (/3-4) restores the heap property "as if by make_heap" with postcondition
// is_heap(c.begin(), c.end(), comp). top() returns c.front().
// So a stateful comparator object passed to the constructor (not a default-constructed one) is
// the one used by every operation, including push_range; function pointers work as Compare;
// a comparator that is a strict weak order on a key yields the elements in key order.
#include <queue>
#include <algorithm>
#include <deque>
#include <functional>
#include <vector>
#include "test_iterators.hpp"
#include "check.hpp"

// Orders by (x % mod), descending priority for the largest remainder; ties broken by value so
// the order is total. A default-constructed Mod (mod == 0) is unusable on purpose.
struct Mod {
  int mod = 0;
  int* calls = nullptr;
  constexpr bool operator()(int a, int b) const {
    if (calls) ++*calls;
    if (mod == 0) throw 0;  // would be a default-constructed comparator
    if (a % mod != b % mod) return a % mod < b % mod;
    return a < b;
  }
};

template <class C, class Cmp>
struct Peek : std::priority_queue<int, C, Cmp> {
  using Base = std::priority_queue<int, C, Cmp>;
  using Base::Base;
  constexpr bool heap() const { return std::is_heap(this->c.begin(), this->c.end(), this->comp); }
};

template <class Q>
constexpr bool drains(Q& q, std::initializer_list<int> expect) {
  for (int e : expect) {
    if (q.empty() || q.top() != e || !q.heap()) return false;
    q.pop();
  }
  return q.empty();
}

template <class C>
constexpr bool stateful() {
  int calls = 0;
  Mod m{10, &calls};
  using Q = Peek<C, Mod>;
  Q q(m);
  q.push(21);
  q.push(19);
  q.emplace(35);
  int more[] = {48, 7, 101, 99};
  q.push_range(more);
  if (q.size() != 7 || !q.heap() || calls == 0) return false;
  // remainders: 21->1, 19->9, 35->5, 48->8, 7->7, 101->1, 99->9
  if (!drains(q, {99, 19, 48, 7, 35, 101, 21})) return false;

  int src[] = {13, 40, 26, 5};
  Q r(src, src + 4, Mod{7});  // 13->6, 40->5, 26->5, 5->5
  r.push_range(InputRange<int>{more, more + 4});  // 48->6, 7->0, 101->3, 99->1
  if (!drains(r, {48, 13, 40, 26, 5, 101, 99, 7})) return false;

  Q s(std::from_range, std::vector<int>{3, 14, 25}, Mod{11});  // 3, 3, 3 -> by value
  s.push(36);                                                  // 3
  if (!drains(s, {36, 25, 14, 3})) return false;

  C init{1, 2, 3, 4, 5, 6};
  Q t(Mod{3}, init);  // 2 % 3 == 5 % 3 == 2 first
  return drains(t, {5, 2, 4, 1, 6, 3});
}

constexpr bool by_pointer(int a, int b) { return a > b; }  // smallest first

constexpr bool fn_pointer() {
  using Q = Peek<std::vector<int>, bool (*)(int, int)>;
  Q q(&by_pointer);
  int arr[] = {5, 1, 4};
  q.push_range(arr);
  q.push(3);
  q.emplace(2);
  return drains(q, {1, 2, 3, 4, 5});
}

static_assert(stateful<std::vector<int>>());
static_assert(fn_pointer());

int main() {
  CHECK(stateful<std::vector<int>>());
  CHECK(stateful<std::deque<int>>());
  CHECK(fn_pointer());
  return 0;
}
