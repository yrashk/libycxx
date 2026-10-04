// [basic.string.general]/3: "In all cases, [data(), data() + size()] is a valid range,
// data() + size() points at an object with value charT() (a "null terminator"), and
// size() <= capacity() is true." Checked after every kind of modification, across lengths
// that cross any small-buffer threshold.
#include <string>
#include <utility>
#include "check.hpp"

constexpr bool ok(const std::string& s) {
  return s.data()[s.size()] == '\0' && s.c_str() == s.data() && s.size() <= s.capacity();
}

constexpr bool test() {
  for (std::size_t n = 0; n < 70; n += 3) {
    std::string s(n, 'x');
    if (!ok(s)) return false;
    s.push_back('y');
    if (!ok(s)) return false;
    s.pop_back();
    if (!ok(s)) return false;
    s.resize(n + 5);
    if (!ok(s)) return false;
    s.resize(n / 2);
    if (!ok(s)) return false;
    s.append(n, 'a');
    if (!ok(s)) return false;
    s.insert(0, n, 'b');
    if (!ok(s)) return false;
    s.erase(0, n / 3);
    if (!ok(s)) return false;
    s.replace(0, 1, n, 'c');
    if (!ok(s)) return false;
    s.reserve(3 * n);
    if (!ok(s)) return false;
    s.shrink_to_fit();
    if (!ok(s)) return false;
    std::string t = std::move(s);
    if (!ok(t) || !ok(s)) return false;
    s = t;
    if (!ok(s)) return false;
    s.assign(n, 'd');
    if (!ok(s)) return false;
    s.clear();
    if (!ok(s)) return false;
    s.resize_and_overwrite(n, [](char* p, std::size_t m) {
      for (std::size_t i = 0; i < m; ++i) p[i] = 'e';
      return m;
    });
    if (!ok(s) || s.size() != n) return false;
    s.swap(t);
    if (!ok(s) || !ok(t)) return false;
  }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
