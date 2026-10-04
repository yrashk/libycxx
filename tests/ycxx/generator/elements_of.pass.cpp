// [coro.generator.promise]/10-12: co_yield ranges::elements_of(g) for a generator g with the same
// yielded type (rvalue or lvalue g) pushes g's coroutine on the active stack and resumes it: its
// elements are produced in place; the yield-expression has type void.
// [coro.generator.promise]/13: co_yield ranges::elements_of(r) for any other input range whose
// reference converts to yielded produces static_cast<yielded>(*i) for each element, through a
// nested generator<yielded, void, Alloc> allocated with r.allocator.
// [coro.generator.members]/4 Note 2: destroying the root destroys the whole stack.
// [range.elementsof]: elements_of<R, Allocator = allocator<byte>> and its deduction guide.
#include <generator>
#include <forward_list>
#include <list>
#include <memory>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

std::generator<int> range(int a, int b) {
  for (int i = a; i < b; ++i) co_yield i;
}

std::generator<int> concat() {
  co_yield 0;
  co_yield std::ranges::elements_of(range(1, 3));
  co_yield std::ranges::elements_of(range(3, 3));  // empty
  co_yield 3;
  auto lv = range(4, 6);
  co_yield std::ranges::elements_of(lv);  // lvalue generator
  co_yield 6;
}

struct Node {
  int value;
  std::vector<Node> children;
};
// Recursive pre-order traversal.
std::generator<const int&> preorder(const Node& n) {
  co_yield n.value;
  for (const Node& c : n.children) co_yield std::ranges::elements_of(preorder(c));
}

std::generator<int> deep(int depth) {
  co_yield depth;
  if (depth > 0) co_yield std::ranges::elements_of(deep(depth - 1));
  co_yield -depth;
}

// Different Ref/Val but the same yielded type (int&&): the generator overload applies.
std::generator<int&&> rref_gen() { co_yield 7; }
std::generator<int> from_rref() { co_yield std::ranges::elements_of(rref_gen()); }

// Nested generator yielding references into a container; the outer sees the same objects.
std::generator<int&> refs(std::vector<int>& v) {
  for (int& x : v) co_yield x;
}
std::generator<int&> refs_outer(std::vector<int>& a, std::vector<int>& b) {
  co_yield std::ranges::elements_of(refs(a));
  co_yield std::ranges::elements_of(refs(b));
}

// Arbitrary input ranges.
// (range_reference_t<R> must convert to yielded: for generator<int>, yielded is int&&, so an
// lvalue range of int needs views::as_rvalue, while prvalues and other types convert.)
std::generator<int> from_ranges() {
  std::vector<int> v{1, 2};
  co_yield std::ranges::elements_of(std::views::as_rvalue(v));
  co_yield std::ranges::elements_of(std::list<int>{3, 4} | std::views::as_rvalue);
  co_yield std::ranges::elements_of(std::views::iota(5, 7));
  const std::forward_list<short> fl{7};  // const short& converts to int&& (a temporary)
  co_yield std::ranges::elements_of(fl);
  co_yield std::ranges::elements_of(std::views::iota(0, 0));
}
// For generator<const int&> the lvalue references convert directly.
std::generator<const int&> from_lvalue_ranges() {
  std::vector<int> v{1, 2};
  co_yield std::ranges::elements_of(v);
  const std::list<int> l{3};
  co_yield std::ranges::elements_of(l);
  co_yield std::ranges::elements_of(std::views::iota(4, 5));  // prvalue: bound to a temporary
}

std::generator<std::string> strings() {
  std::vector<std::string> v{"a", "b"};
  co_yield std::ranges::elements_of(v | std::views::transform([](const std::string& s) { return s; }));  // copies
  co_yield std::ranges::elements_of(v | std::views::transform([](const std::string& s) { return s + "!"; }));
  co_yield std::ranges::elements_of(std::views::as_rvalue(v));  // moves
}

int counted_allocs = 0;
template <class T>
struct CountAlloc {
  using value_type = T;
  CountAlloc() = default;
  template <class U>
  CountAlloc(const CountAlloc<U>&) noexcept {}
  T* allocate(std::size_t n) {
    ++counted_allocs;
    return std::allocator<T>{}.allocate(n);
  }
  void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>{}.deallocate(p, n); }
  template <class U>
  friend bool operator==(const CountAlloc&, const CountAlloc<U>&) noexcept { return true; }
};
std::generator<int> with_alloc() {
  co_yield std::ranges::elements_of(std::views::iota(1, 4), CountAlloc<std::byte>());
}

int alive = 0;
struct Guard {
  Guard() { ++alive; }
  ~Guard() { --alive; }
};
std::generator<int> inner_guarded() {
  Guard g;
  co_yield 1;
  co_yield 2;
}
std::generator<int> outer_guarded() {
  Guard g;
  co_yield std::ranges::elements_of(inner_guarded());
}

template <class G>
std::vector<std::ranges::range_value_t<G>> collect(G&& g) {
  std::vector<std::ranges::range_value_t<G>> out;
  for (auto&& x : g) out.push_back(x);
  return out;
}

// [range.elementsof] deduction guide.
static_assert(std::is_same_v<decltype(std::ranges::elements_of(std::declval<std::vector<int>&>())),
                             std::ranges::elements_of<std::vector<int>&, std::allocator<std::byte>>>);
static_assert(std::is_same_v<decltype(std::ranges::elements_of(std::vector<int>())),
                             std::ranges::elements_of<std::vector<int>&&, std::allocator<std::byte>>>);
static_assert(std::is_same_v<decltype(std::ranges::elements_of(std::declval<std::vector<int>&>(), CountAlloc<int>())),
                             std::ranges::elements_of<std::vector<int>&, CountAlloc<int>>>);

int main() {
  CHECK((collect(concat()) == std::vector<int>{0, 1, 2, 3, 4, 5, 6}));
  Node tree{1, {{2, {{3, {}}, {4, {}}}}, {5, {}}, {6, {{7, {{8, {}}}}}}}};
  CHECK((collect(preorder(tree)) == std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8}));
  {
    // References yielded by nested generators refer to the tree's own ints.
    auto g = preorder(tree);
    auto it = g.begin();
    ++it;
    ++it;
    CHECK(&*it == &tree.children[0].children[0].value);
  }
  auto d = collect(deep(200));
  CHECK(d.size() == 402 && d.front() == 200 && d[200] == 0 && d[201] == 0 && d.back() == -200);
  CHECK((collect(from_rref()) == std::vector<int>{7}));
  {
    std::vector<int> a{1, 2}, b{3};
    for (int& x : refs_outer(a, b)) x = -x;
    CHECK((a == std::vector<int>{-1, -2}) && (b == std::vector<int>{-3}));
  }
  CHECK((collect(from_ranges()) == std::vector<int>{1, 2, 3, 4, 5, 6, 7}));
  CHECK((collect(from_lvalue_ranges()) == std::vector<int>{1, 2, 3, 4}));
  CHECK((collect(strings()) == std::vector<std::string>{"a", "b", "a!", "b!", "a", "b"}));
  {
    counted_allocs = 0;
    CHECK((collect(with_alloc()) == std::vector<int>{1, 2, 3}));
    CHECK(counted_allocs >= 1);  // the nested generator's frame came from the allocator
  }
  {
    // Destroying the root while a nested generator is suspended destroys both frames.
    alive = 0;
    {
      auto g = outer_guarded();
      auto it = g.begin();
      CHECK(*it == 1 && alive == 2);
    }
    CHECK(alive == 0);
    {
      auto g = outer_guarded();
      for (int x : g) (void)x;
      CHECK(alive == 0);
    }
  }
  return 0;
}
