// [string.cons]/13-15: basic_string(const charT* s, size_type n, a): value [s, s + n);
// size() == n (embedded nulls kept). /16-17: basic_string(const charT* s, a) is
// basic_string(s, traits::length(s), a). /18-19: basic_string(size_type n, charT c, a):
// n copies of c. /23: initializer_list constructor.
#include <string>
#include "check.hpp"

constexpr bool test() {
  {
    std::string s("abc\0def", 7);
    if (s.size() != 7 || s[3] != '\0' || s[6] != 'f' || s.data()[7] != '\0') return false;
  }
  {
    std::string s("abc\0def");
    if (s.size() != 3 || s != "abc") return false;
  }
  {
    std::string s("", 0);
    if (!s.empty()) return false;
  }
  {
    std::string s(5, 'x');
    if (s != "xxxxx") return false;
    std::string z(0, 'x');
    if (!z.empty()) return false;
    std::string big(1000, 'q');
    if (big.size() != 1000 || big[999] != 'q' || big.c_str()[1000] != '\0') return false;
  }
  {
    std::string s{'a', 'b', 'c'};
    if (s != "abc") return false;
    std::string e(std::initializer_list<char>{});
    if (!e.empty()) return false;
  }
  {
    // brace-init with two ints picks the initializer_list constructor
    std::string s{65, 66};
    if (s != "AB") return false;
    // parentheses pick (size_type n, charT c)
    std::string t(3, 65);
    if (t != "AAA") return false;
  }
  {
    std::u32string s(U"\U0001F600x");
    if (s.size() != 2 || s[0] != U'\U0001F600') return false;
    std::wstring w(L"wide", 2);
    if (w != L"wi") return false;
  }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
