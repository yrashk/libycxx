// [vector.modifiers]/2: "If an exception is thrown other than by the copy constructor, move
// constructor, assignment operator, or move assignment operator of T or by any
// InputIterator operation, there are no effects. If an exception is thrown while inserting
// a single element at the end and T is Cpp17CopyInsertable or
// is_nothrow_move_constructible_v<T> is true, there are no effects."
// [vector.capacity]/4,9,16,19: reserve / shrink_to_fit / resize leave no effects on an
// exception (other than from a non-copyable T's move constructor).
// [container.reqmts]/66: push_back / emplace_back have no effects if they throw.
// REQUIRES: exceptions
#include <vector>
#include <new>
#include <stdexcept>
#include "test_allocators.hpp"
#include "check.hpp"

// Copyable type whose copies can be armed to throw. Relocation strategy is unspecified;
// the inserted lvalue independently requires a copy.
struct ThrowingCopy {
  static inline int copies_until_throw = -1;
  static inline int moves = 0;
  int v;
  ThrowingCopy(int x) : v(x) {}
  ThrowingCopy(const ThrowingCopy& o) : v(o.v) {
    if (copies_until_throw == 0) throw std::runtime_error("copy");
    if (copies_until_throw > 0) --copies_until_throw;
  }
  ThrowingCopy(ThrowingCopy&& o) noexcept(false) : v(o.v) { ++moves; }
  ThrowingCopy& operator=(const ThrowingCopy&) = default;
};

template <class V>
bool same(const V& a, const std::vector<int>& expect) {
  if (a.size() != expect.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i)
    if (a[i].v != expect[i]) return false;
  return true;
}

int main() {
  {
    // A full vector must grow, but the inserted lvalue's copy throws independently
    // of whether an implementation copies or moves existing elements.
    std::vector<ThrowingCopy> v;
    v.reserve(3);
    std::vector<int> expected;
    while (v.size() < v.capacity()) {
      int i = static_cast<int>(v.size());
      v.emplace_back(i);
      expected.push_back(i);
    }
    const ThrowingCopy* data = v.data();
    auto cap = v.capacity();
    ThrowingCopy::copies_until_throw = 0;
    bool threw = false;
    try {
      v.push_back(v.front());  // also exercises aliasing an existing element
    } catch (const std::runtime_error&) {
      threw = true;
    }
    ThrowingCopy::copies_until_throw = -1;
    CHECK(threw && same(v, expected));
    CHECK(v.capacity() == cap && v.data() == data);
    v.push_back(v.front());
    expected.push_back(0);
    CHECK(same(v, expected));
  }
  {
    // reserve must grow; copying or moving is permitted. If a copy throws, no effects.
    std::vector<ThrowingCopy> v;
    for (int i = 0; i < 4; ++i) v.emplace_back(i);
    auto cap = v.capacity();
    ThrowingCopy::copies_until_throw = 1;
    bool threw = false;
    try {
      v.reserve(cap + 10);
    } catch (const std::runtime_error&) {
      threw = true;
    }
    ThrowingCopy::copies_until_throw = -1;
    CHECK(same(v, {0, 1, 2, 3}));
    CHECK(threw ? v.capacity() == cap : v.capacity() >= cap + 10);
  }
  {
    // resize(n, c) has no effects on an exception ([vector.capacity]/19).
    std::vector<ThrowingCopy> v;
    for (int i = 0; i < 2; ++i) v.emplace_back(i);
    ThrowingCopy::copies_until_throw = 3;
    bool threw = false;
    try {
      v.resize(20, ThrowingCopy(7));
    } catch (const std::runtime_error&) {
      threw = true;
    }
    ThrowingCopy::copies_until_throw = -1;
    CHECK(threw && same(v, {0, 1}));
  }
  {
    // Allocation failure: insert in the middle, insert of a range, push_back, emplace_back,
    // reserve, resize, assign-like growth: no effects.
    using V = std::vector<int, CountingAlloc<int>>;
    V v{1, 2, 3};
    v.shrink_to_fit();
    int extra[] = {7, 8, 9, 10, 11, 12, 13, 14};
    while (v.size() < v.capacity()) v.push_back(0);  // make every growth reallocate
    const V full = v;
    auto fails_full = [&](auto f) {
      alloc_counters.fail_after = 0;
      bool threw = false;
      try {
        f();
      } catch (const std::bad_alloc&) {
        threw = true;
      }
      alloc_counters.fail_after = -1;
      return threw && v == full;
    };
    CHECK(fails_full([&] { v.push_back(4); }));
    CHECK(fails_full([&] { v.emplace_back(4); }));
    CHECK(fails_full([&] { v.insert(v.begin() + 1, 5); }));
    CHECK(fails_full([&] { v.emplace(v.begin(), 5); }));
    CHECK(fails_full([&] { v.insert(v.begin() + 1, 10, 5); }));
    CHECK(fails_full([&] { v.insert(v.begin(), extra, extra + 8); }));
    CHECK(fails_full([&] { v.insert_range(v.begin() + 2, extra); }));
    CHECK(fails_full([&] { v.append_range(extra); }));
    CHECK(fails_full([&] { v.reserve(v.capacity() + 100); }));
    CHECK(fails_full([&] { v.resize(v.capacity() + 100); }));
    CHECK(fails_full([&] { v.resize(v.capacity() + 100, 3); }));
  }
  CHECK(alloc_counters.outstanding == 0);
  return 0;
}
