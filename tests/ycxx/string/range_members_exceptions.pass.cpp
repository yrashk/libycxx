// [string.require]/2: "If any member function or operator of basic_string throws an
// exception, that function or operator has no other effect on the basic_string object."
// Checked for the range members and the iterator-pair members when the source range throws
// part-way through (a single-pass input range whose iterator throws on the k-th dereference),
// and for resize_and_overwrite and the range members when the allocator throws:
// [string.append] append_range, append(first, last); [string.insert] insert_range,
// insert(p, first, last); [string.replace] replace_with_range, replace(i1, i2, j1, j2);
// [string.assign] assign_range, assign(first, last); [string.capacity]/7-10
// resize_and_overwrite; [string.cons] basic_string(from_range, rg) propagates the exception
// and frees what it allocated ([string.require]/3: storage comes from the allocator).
#include <string>
#include <cstddef>
#include <iterator>
#include <new>
#include <ranges>
#include "test_allocators.hpp"
#include "check.hpp"

struct Boom {};

// Single-pass input iterator over "0123456789..." that throws on the dereference numbered
// *throw_at (0-based, counted through *count).
struct ThrowingIter {
  using value_type = char;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::input_iterator_tag;
  using reference = char;
  using pointer = void;
  int pos = 0;
  int* count = nullptr;
  int throw_at = -1;
  char operator*() const {
    if ((*count)++ == throw_at) throw Boom{};
    return static_cast<char>('0' + pos % 10);
  }
  ThrowingIter& operator++() {
    ++pos;
    return *this;
  }
  void operator++(int) { ++pos; }
  friend bool operator==(const ThrowingIter& a, const ThrowingIter& b) { return a.pos == b.pos; }
};
static_assert(std::input_iterator<ThrowingIter>);

struct ThrowingRange {  // input_range, not sized, common
  int n;
  int throw_at;
  int* count;
  ThrowingIter begin() const { return ThrowingIter{0, count, throw_at}; }
  ThrowingIter end() const { return ThrowingIter{n, count, throw_at}; }
};
static_assert(std::ranges::input_range<ThrowingRange> && !std::ranges::forward_range<ThrowingRange>);

using CS = std::basic_string<char, std::char_traits<char>, CountingAlloc<char>>;

template <class Str, class F>
bool throws_unchanged(Str& s, F f) {
  const Str before = s;
  const auto cap = s.capacity();
  bool threw = false;
  try {
    f();
  } catch (const Boom&) {
    threw = true;
  }
  return threw && s == before && s.capacity() == cap && s.c_str()[s.size()] == '\0';
}

template <class Str, class F>
bool bad_alloc_unchanged(Str& s, F f) {
  const Str before = s;
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

template <class Str>
void run(Str& s) {
  int count = 0;
  for (int n : {5, 100}) {
    for (int k : {0, 3, n - 1}) {
      ThrowingRange rg{n, k, &count};
      count = 0;
      CHECK(throws_unchanged(s, [&] { s.append_range(rg); }));
      count = 0;
      CHECK(throws_unchanged(s, [&] { s.append(rg.begin(), rg.end()); }));
      count = 0;
      CHECK(throws_unchanged(s, [&] { s.insert_range(s.begin() + 1, rg); }));
      count = 0;
      CHECK(throws_unchanged(s, [&] { s.insert(s.begin() + 2, rg.begin(), rg.end()); }));
      count = 0;
      CHECK(throws_unchanged(s, [&] { s.replace_with_range(s.begin() + 1, s.begin() + 3, rg); }));
      count = 0;
      CHECK(throws_unchanged(s, [&] { s.replace(s.begin(), s.end(), rg.begin(), rg.end()); }));
      count = 0;
      CHECK(throws_unchanged(s, [&] { s.assign_range(rg); }));
      count = 0;
      CHECK(throws_unchanged(s, [&] { s.assign(rg.begin(), rg.end()); }));
    }
  }
  // control: without a throw, the members work on this range type
  ThrowingRange ok{12, -1, &count};
  Str t = s;
  t.append_range(ok);
  CHECK(t.size() == s.size() + 12 && t.back() == '1');
  t.assign_range(ok);
  CHECK(t == "012345678901");
}

int main() {
  std::string small = "abcd";
  run(small);
  std::string big(300, 'q');
  run(big);

  // from_range construction: the exception propagates and nothing leaks
  {
    int count = 0;
    alloc_counters = AllocCounters{};
    bool threw = false;
    try {
      CS s(std::from_range, ThrowingRange{500, 400, &count});
    } catch (const Boom&) {
      threw = true;
    }
    CHECK(threw);
    CHECK(alloc_counters.outstanding == 0);
    CHECK(alloc_counters.allocations == alloc_counters.deallocations);
  }

  // allocator failure
  {
    CS s("a string long enough to live in allocated storage, for sure, surely");
    const auto big_n = s.capacity() + 1;
    int count = 0;
    ThrowingRange rg{static_cast<int>(big_n), -1, &count};
    CHECK(bad_alloc_unchanged(s, [&] { s.append_range(rg); }));
    CHECK(bad_alloc_unchanged(s, [&] { s.insert_range(s.begin(), rg); }));
    CHECK(bad_alloc_unchanged(s, [&] { s.replace_with_range(s.begin(), s.begin() + 1, rg); }));
    CHECK(bad_alloc_unchanged(s, [&] { s.assign_range(rg); }));
    CHECK(bad_alloc_unchanged(s, [&] { s.append(rg.begin(), rg.end()); }));
    CHECK(bad_alloc_unchanged(s, [&] {
      s.resize_and_overwrite(big_n + 10, [](char* p, std::size_t n) {
        for (std::size_t i = 0; i < n; ++i) p[i] = 'w';
        return n;
      });
    }));
  }
  return 0;
}
