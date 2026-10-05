// [basic.string.general]: every member of basic_string is constexpr, and so are the
// non-member operator+, comparisons, swap and erase/erase_if ([string.syn]); a string may be
// created, grown past any small-buffer size, modified and destroyed during constant
// evaluation (transient allocation). [basic.string.general]/5: iterator and const_iterator
// meet the constexpr iterator requirements.
// COUNTERPART: libstdcxx:21_strings/basic_string/cons/(char|wchar_t)/constexpr.cc
#include <string>
#include <algorithm>
#include <string_view>
#include "check.hpp"

constexpr std::string build(int n) {
  std::string s;
  for (int i = 0; i < n; ++i) s += static_cast<char>('a' + i % 26);
  return s;
}

constexpr bool test() {
  std::string s = build(300);
  if (s.size() != 300 || s[26] != 'a' || s.back() != static_cast<char>('a' + 299 % 26)) return false;
  s.erase(10);
  s.insert(0, "<<");
  s.replace(2, 3, "R");
  s.append(5, '!');
  if (s != "<<Rdefghij!!!!!") return false;
  std::reverse(s.begin(), s.end());
  if (s.find("jihgfed") != 5) return false;
  std::string t = s.substr(5, 3) + std::string_view("-") + 'x';
  if (t != "jih-x") return false;
  std::string u;
  u.reserve(1000);
  u = std::move(t);
  t = "reassigned";
  swap(t, u);
  if (t != "jih-x" || u != "reassigned") return false;
  if (std::erase(u, 'e') != 2 || u != "rassignd") return false;
  if (std::erase_if(u, [](char c) { return c == 's'; }) != 2 || u != "raignd") return false;
  std::wstring w(40, L'w');
  w.resize(80, L'v');
  if (w[79] != L'v' || w.find(L'v') != 40) return false;
  return true;
}
static_assert(test());

// A constexpr string result can be used to compute a constant (not stored in it).
static_assert(build(100).size() == 100);
static_assert(build(5) == "abcde");
static_assert((std::string("a") + "b" + std::string(3, 'c')).size() == 5);

int main() {
  CHECK(test());
  CHECK(build(60).size() == 60);
  return 0;
}
