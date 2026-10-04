// [string.erase]/1-3: erase(pos = 0, n = npos) removes min(n, size() - pos) characters from
// pos, returns *this, throws out_of_range if pos > size(). /4-7: erase(p) returns an
// iterator to the element following p (or end()); throws nothing. /8-11: erase(first,
// last) returns an iterator to the element last pointed to; throws nothing. /12-14:
// pop_back() is erase(end() - 1). [string.capacity]/19: clear() is erase(begin(), end()).
#include <string>
#include <stdexcept>
#include <utility>
#include "check.hpp"

static_assert(noexcept(std::declval<std::string&>().clear()));

constexpr bool test() {
  std::string s = "0123456789";
  if (&s.erase(8) != &s || s != "01234567") return false;
  s.erase(2, 3);
  if (s != "01567") return false;
  s.erase(1, 100);
  if (s != "0") return false;
  s.erase(1);  // pos == size()
  if (s != "0") return false;
  s.erase();
  if (!s.empty() || s.c_str()[0] != '\0') return false;

  s = "abcde";
  auto it = s.erase(s.cbegin() + 1);
  if (s != "acde" || it != s.begin() + 1 || *it != 'c') return false;
  it = s.erase(s.cend() - 1);
  if (s != "acd" || it != s.end()) return false;
  it = s.erase(s.cbegin(), s.cbegin() + 2);
  if (s != "d" || it != s.begin()) return false;
  it = s.erase(s.cbegin(), s.cbegin());
  if (s != "d" || it != s.begin()) return false;
  it = s.erase(s.cbegin(), s.cend());
  if (!s.empty() || it != s.end()) return false;

  s = "xyz";
  s.pop_back();
  if (s != "xy" || s.data()[2] != '\0') return false;
  s.clear();
  if (!s.empty() || s.data()[0] != '\0') return false;

  std::string big(200, 'q');
  auto cap = big.capacity();
  big.clear();
  if (!big.empty()) return false;
  (void)cap;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::string s = "abc";
  bool threw = false;
  try {
    s.erase(4);
  } catch (const std::out_of_range&) {
    threw = true;
  }
  CHECK(threw && s == "abc");
  return 0;
}
