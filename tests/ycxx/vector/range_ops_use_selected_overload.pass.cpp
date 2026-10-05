// [sequence.reqmts]: X(i, j) "Constructs a sequence container equal to the range [i, j)" with
// "T is Cpp17EmplaceConstructible into X from *i"; X(from_range, rg) likewise from
// *ranges::begin(rg); a.insert(p, i, j) / a.insert_range(p, rg) / a.append_range(rg) /
// a.prepend_range(rg) "Inserts copies of elements in [i, j) / rg"; a.assign(i, j) /
// a.assign_range(rg) "Replaces elements in a with a copy of" them (EmplaceConstructible from
// and assignable from *i); [container.alloc.reqmts]/2.6: emplace-constructing evaluates
// allocator_traits<A>::construct(m, p, args) with args = *i; ranges::to constructs the
// container from the range ([range.utility.conv.to]). When *i is a non-const lvalue S&,
// overload resolution selects a constructor (or assignment) template taking U& (U = S) over the
// defaulted copy operations, so the copies are made by those templates even though S is
// trivially copyable; they mark the value (+1000). A byte copy of the source leaves the values
// unmarked. (Moves of existing elements may use the defaulted operations, so only the copied
// elements are checked, and only for being marked.)
#include <concepts>
#include <cstddef>
#include <deque>
#include <iterator>
#include <list>
#include <ranges>
#include <type_traits>
#include <vector>
#include "check.hpp"

struct S {
  int v;
  S(int x = 0) : v(x) {}
  S(const S&) = default;
  S& operator=(const S&) = default;
  template <class U>
    requires std::same_as<U, S>
  S(U& o) : v(o.v + 1000) {}
  template <class U>
    requires std::same_as<U, S>
  S& operator=(U& o) {
    v = o.v + 1000;
    return *this;
  }
};
static_assert(std::is_trivially_copyable_v<S>);
static_assert(!std::is_trivially_constructible_v<S, S&>);

constexpr int N = 24;

// a copy of src[i] made through the templates (once or more), never a byte copy
static bool marked(const S& s, int i) { return s.v != i && s.v > i && (s.v - i) % 1000 == 0; }

template <class C>
void expect_copies(const C& c, std::size_t at, int n) {
  CHECK(c.size() >= at + static_cast<std::size_t>(n));
  auto it = c.begin();
  std::advance(it, static_cast<std::ptrdiff_t>(at));
  for (int i = 0; i < n; ++i, ++it) CHECK(marked(*it, i));
}

template <class C>
void run() {
  S src[N];
  for (int i = 0; i < N; ++i) src[i].v = i;
  for (int n : {1, 5, N}) {
    auto rg = std::ranges::subrange(src, src + n);
    expect_copies(C(src, src + n), 0, n);
    expect_copies(C(std::from_range, rg), 0, n);
    expect_copies(std::ranges::to<C>(rg), 0, n);
    for (int old : {0, 3, 40}) {  // growing and shrinking assignments
      C b(static_cast<std::size_t>(old), S(-1));
      b.assign(src, src + n);
      CHECK(b.size() == static_cast<std::size_t>(n));
      expect_copies(b, 0, n);
      C b2(static_cast<std::size_t>(old), S(-1));
      b2.assign_range(rg);
      CHECK(b2.size() == static_cast<std::size_t>(n));
      expect_copies(b2, 0, n);
    }
    for (std::size_t at : {0u, 1u, 3u}) {
      C c(3, S(-5));
      if constexpr (requires { c.reserve(1); }) c.reserve(100);
      auto pos = c.begin();
      std::advance(pos, static_cast<std::ptrdiff_t>(at));
      c.insert(pos, src, src + n);
      expect_copies(c, at, n);
      C c2(3, S(-5));
      pos = c2.begin();
      std::advance(pos, static_cast<std::ptrdiff_t>(at));
      c2.insert_range(pos, rg);
      expect_copies(c2, at, n);
    }
    C d(2, S(-7));
    d.append_range(rg);
    expect_copies(d, 2, n);
    if constexpr (requires { d.prepend_range(rg); }) {
      C e(2, S(-7));
      e.prepend_range(rg);
      expect_copies(e, 0, n);
    }
  }
}

int main() {
  run<std::vector<S>>();
  run<std::deque<S>>();
  run<std::list<S>>();
}
