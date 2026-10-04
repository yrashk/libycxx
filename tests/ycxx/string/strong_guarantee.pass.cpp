// [string.require]/2: "If any member function or operator of basic_string throws an
// exception, that function or operator has no other effect on the basic_string object."
// Exercised with an allocator whose allocate() throws bad_alloc ([string.insert]/10.3,
// [string.replace]/10.3: exceptions thrown by allocator_traits<Allocator>::allocate
// propagate).
#include <string>
#include <new>
#include "test_allocators.hpp"
#include "check.hpp"

using S = std::basic_string<char, std::char_traits<char>, CountingAlloc<char>>;

template <class F>
bool bad_alloc_and_unchanged(S& s, F f) {
  const S before = s;
  const auto cap = s.capacity();
  alloc_counters.fail_after = 0;
  bool threw = false;
  try {
    f();
  } catch (const std::bad_alloc&) {
    threw = true;
  }
  alloc_counters.fail_after = -1;
  return threw && s == before && s.capacity() == cap && s.c_str()[s.size()] == '\0';
}

int main() {
  {
    S s("a string that already needs a heap allocation of some size, surely");
    const auto big = s.capacity() + 1;
    CHECK(bad_alloc_and_unchanged(s, [&] { s.append(big, 'x'); }));
    CHECK(bad_alloc_and_unchanged(s, [&] { s.insert(3, big, 'x'); }));
    CHECK(bad_alloc_and_unchanged(s, [&] { s.insert(s.cbegin() + 1, big, 'x'); }));
    CHECK(bad_alloc_and_unchanged(s, [&] { s.replace(1, 2, big, 'x'); }));
    CHECK(bad_alloc_and_unchanged(s, [&] { s.reserve(big + 100); }));
    CHECK(bad_alloc_and_unchanged(s, [&] { s.resize(big + 100, 'r'); }));
    CHECK(bad_alloc_and_unchanged(s, [&] { s += S(big, 'q'); }));
    CHECK(bad_alloc_and_unchanged(s, [&] { s.assign(big, 'z'); }));
    S other(big, 'o');
    CHECK(bad_alloc_and_unchanged(s, [&] { s = other; }));
  }
  {
    // A single push_back that must reallocate leaves the string unchanged on failure.
    S s("0123456789abcdefghijklmnopqrstuvwxyz0123456789");
    while (s.size() < s.capacity()) s.push_back('f');
    CHECK(bad_alloc_and_unchanged(s, [&] { s.push_back('!'); }));
  }
  CHECK(alloc_counters.outstanding == 0);
  return 0;
}
