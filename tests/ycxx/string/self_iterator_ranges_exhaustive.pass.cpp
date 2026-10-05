// The iterator-pair members of basic_string with a source range inside *this, for every
// source window and destination position of short strings and a sample of long ones, with
// exact and spare capacity, through the string's own iterators, reverse iterators and an
// input-only iterator. [string.insert]/21-24 insert(p, first, last): "Equivalent to insert(p -
// begin(), basic_string(first, last, get_allocator()))"; [string.append]/14-15 and
// [string.assign]/15-16: "Equivalent to: return append(basic_string(first, last,
// get_allocator()))" / "assign(basic_string(first, last, get_allocator()))"; [string.replace]
// replace(i1, i2, j1, j2): "Equivalent to: return replace(i1, i2, basic_string(j1, j2,
// get_allocator()))". The source characters are therefore read as they were before the call,
// even when [first, last) overlaps the part of *this that is moved or reallocated.
#include <string>
#include <iterator>
#include <cstddef>
#include "test_iterators.hpp"
#include "check.hpp"

using S = std::string;

static S make(std::size_t len, bool spare) {
  S s;
  for (std::size_t i = 0; i < len; ++i) s.push_back(static_cast<char>('a' + i % 26));
  if (spare) s.reserve(3 * len + 8);
  else s.shrink_to_fit();
  return s;
}

static S rev(S v) { return S(v.rbegin(), v.rend()); }

static void run(std::size_t len, std::size_t step) {
  for (int spare = 0; spare < 2; ++spare) {
    const S v = make(len, false);
    for (std::size_t b = 0; b <= len; b += step) {
      for (std::size_t e = b; e <= len; e += step) {
        const S win = v.substr(b, e - b);
        auto d = [](S& s, std::size_t i) { return s.begin() + static_cast<std::ptrdiff_t>(i); };
        {
          S s = make(len, spare);
          s.append(d(s, b), d(s, e));
          CHECK(s == v + win);
          s = make(len, spare);
          s.append(InputIter<char>(s.data() + b), InputIter<char>(s.data() + e));
          CHECK(s == v + win);
          s = make(len, spare);
          s.append(std::make_reverse_iterator(d(s, e)), std::make_reverse_iterator(d(s, b)));
          CHECK(s == v + rev(win));
        }
        {
          S s = make(len, spare);
          s.assign(d(s, b), d(s, e));
          CHECK(s == win);
          s = make(len, spare);
          s.assign(InputIter<char>(s.data() + b), InputIter<char>(s.data() + e));
          CHECK(s == win);
          s = make(len, spare);
          s.assign(std::make_reverse_iterator(d(s, e)), std::make_reverse_iterator(d(s, b)));
          CHECK(s == rev(win));
        }
        for (std::size_t p = 0; p <= len; p += step) {
          const S ins = v.substr(0, p) + win + v.substr(p);
          S s = make(len, spare);
          auto r = s.insert(d(s, p), d(s, b), d(s, e));
          CHECK(s == ins && r == d(s, p));
          s = make(len, spare);
          s.insert(d(s, p), InputIter<char>(s.data() + b), InputIter<char>(s.data() + e));
          CHECK(s == ins);
          s = make(len, spare);
          s.insert(d(s, p), std::make_reverse_iterator(d(s, e)), std::make_reverse_iterator(d(s, b)));
          CHECK(s == v.substr(0, p) + rev(win) + v.substr(p));
          // replace [p, q) with the window, for a few q
          for (std::size_t q = p; q <= len; q += step + 2) {
            const S rep = v.substr(0, p) + win + v.substr(q);
            s = make(len, spare);
            s.replace(d(s, p), d(s, q), d(s, b), d(s, e));
            CHECK(s == rep);
            s = make(len, spare);
            s.replace(d(s, p), d(s, q), InputIter<char>(s.data() + b), InputIter<char>(s.data() + e));
            CHECK(s == rep);
          }
        }
      }
    }
  }
}

int main() {
  for (std::size_t len = 0; len <= 8; ++len) run(len, 1);
  run(15, 2);
  run(16, 3);
  run(23, 2);
  run(24, 5);
  run(40, 7);
  run(100, 19);
  return 0;
}
