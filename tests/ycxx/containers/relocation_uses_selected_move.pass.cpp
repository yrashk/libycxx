// When an insertion or erasure in the middle of vector, deque or inplace_vector moves the
// existing elements to other positions, it does so with T's move operations
// ([sequence.reqmts]: insert, emplace and erase require T to be Cpp17MoveInsertable and
// Cpp17MoveAssignable); [vector.modifiers] erase: "the assignment operator of T is called the
// number of times equal to the number of elements in the vector after the erased elements";
// [deque.modifiers] erase: assignments no more than the lesser of the elements before and
// after; [inplace.vector.modifiers] likewise. M is trivially copyable, but for an rvalue M its
// constructor and assignment templates taking U&& (U = M) are better matches than the
// defaulted copy operations; they add 2000 to the value. An element now held at an address
// where a different element was before has been moved there by one of them, so its value
// carries a mark; a byte-wise shift leaves it unmarked. (No reallocation happens: capacity is
// reserved first; deque may move either side.) std::erase / erase_if ([vector.erasure],
// [deque.erasure]) go through remove/remove_if, which move by assignment as well.
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <deque>
#include <inplace_vector>
#include <iterator>
#include <map>
#include <type_traits>
#include <vector>
#include "check.hpp"

struct M {
  int v;
  M(int x = 0) : v(x) {}
  M(const M&) = default;
  M& operator=(const M&) = default;
  template <class U>
    requires std::same_as<U, M>
  M(U&& o) noexcept : v(o.v + 2000) {}
  template <class U>
    requires std::same_as<U, M>
  M& operator=(U&& o) noexcept {
    v = o.v + 2000;
    return *this;
  }
};
static_assert(std::is_trivially_copyable_v<M>);
static_assert(!std::is_trivially_move_constructible_v<M> && !std::is_trivially_move_assignable_v<M>);

constexpr int N = 30;

// an element's identity: its value modulo 2000 (marks are multiples of 2000); the original
// elements are 0..N-1, the inserted ones negative (so 2000 - k)
static int id(int v) { return ((v % 2000) + 2000) % 2000; }
static int moves(int v) { return (v - id(v)) / 2000; }

template <class C>
std::map<const void*, int> addresses(const C& c) {
  std::map<const void*, int> m;
  for (const M& e : c) m[&e] = id(e.v);
  return m;
}

// every original element that sits where another element (or nothing) was before has been
// moved there
template <class C>
void expect_moved(const C& c, const std::map<const void*, int>& before) {
  for (const M& e : c) {
    if (id(e.v) >= 1000) continue;  // an inserted element
    auto it = before.find(&e);
    if (it == before.end() || it->second != id(e.v)) CHECK(moves(e.v) >= 1);
  }
}

// erase(c, value) compares with ==
bool operator==(const M& a, const M& b) { return id(a.v) == id(b.v); }

// c holds 0..N-1, unmarked, with room for more (filled in place: moving the container could
// move the elements)
template <class C>
void fill(C& c) {
  if constexpr (requires { c.reserve(1); }) c.reserve(4 * N);
  for (int i = 0; i < N; ++i) c.push_back(M(i));  // push_back(M&&) marks: reset below
  for (auto& e : c) e.v = id(e.v);
}

template <class C, class Op>
void check_op(Op op) {
  C c;
  fill(c);
  const auto before = addresses(c);
  op(c);
  expect_moved(c, before);
}

template <class C>
void run() {
  const M nv(-1);
  const M three[3] = {-3, -3, -3};
  for (int pos : {0, 1, N / 2, N - 1, N}) {
    check_op<C>([&](C& c) { c.insert(std::next(c.begin(), pos), nv); });
    check_op<C>([&](C& c) { c.emplace(std::next(c.begin(), pos), -2); });
    check_op<C>([&](C& c) { c.insert(std::next(c.begin(), pos), three, three + 3); });
    check_op<C>([&](C& c) { c.insert(std::next(c.begin(), pos), 4, nv); });
    if (pos == N) continue;
    check_op<C>([&](C& c) {
      c.erase(std::next(c.begin(), pos));
      CHECK(static_cast<int>(c.size()) == N - 1);
      if constexpr (!requires { c.push_front(nv); }) {
        // vector, inplace_vector: exactly the elements after the erased one are assigned, once
        for (int i = pos; i < N - 1; ++i) CHECK(c[static_cast<std::size_t>(i)].v == i + 1 + 2000);
      }
    });
    check_op<C>([&](C& c) { c.erase(std::next(c.begin(), pos), std::next(c.begin(), std::min(N, pos + 5))); });
  }
  check_op<C>([](C& c) {
    std::erase_if(c, [](const M& m) { return id(m.v) % 3 == 0; });
    CHECK(static_cast<int>(c.size()) == N - N / 3);
  });
  check_op<C>([](C& c) { CHECK(std::erase(c, M(7)) == 1); });
}

int main() {
  run<std::vector<M>>();
  run<std::deque<M>>();
  run<std::inplace_vector<M, 4 * N>>();
}
