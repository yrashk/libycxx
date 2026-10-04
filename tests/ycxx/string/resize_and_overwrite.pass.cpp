// [string.capacity]/7-11: resize_and_overwrite(n, op): with o = size(), k = min(o, n),
// op is called as std::move(op)(p, m) where [p, p + n] is valid, the first k characters equal
// the old contents, and m == n; the contents become [p, p + r) where r is op's result.
// [basic.string.general]/3: afterwards data()[size()] is the null terminator.
#include <string>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct MoveCalled {
  bool* rvalue_called;
  constexpr std::size_t operator()(char*, std::size_t) & { return 0; }
  constexpr std::size_t operator()(char* p, std::size_t n) && {
    *rvalue_called = true;
    for (std::size_t i = 0; i < n; ++i) p[i] = 'm';
    return n;
  }
};

constexpr bool test() {
  {
    std::string s;
    s.resize_and_overwrite(5, [](char* p, std::size_t n) {
      for (std::size_t i = 0; i < n; ++i) p[i] = static_cast<char>('a' + i);
      return n;
    });
    if (s != "abcde" || s.size() != 5 || s.data()[5] != '\0') return false;
  }
  {
    // Old contents are visible in the first min(o, n) characters; result may be shorter.
    std::string s = "hello";
    bool saw_old = false;
    s.resize_and_overwrite(10, [&](char* p, std::size_t n) {
      saw_old = n == 10 && p[0] == 'h' && p[4] == 'o';
      p[5] = '!';
      return std::size_t(6);
    });
    if (!saw_old || s != "hello!" || s.c_str()[6] != '\0') return false;
  }
  {
    // Shrinking n: only the first n old characters are guaranteed, result r <= n.
    std::string s = "abcdefgh";
    s.resize_and_overwrite(3, [](char* p, std::size_t n) {
      return (n == 3 && p[0] == 'a' && p[2] == 'c') ? std::size_t(2) : std::size_t(0);
    });
    if (s != "ab") return false;
  }
  {
    std::string s = "abc";
    s.resize_and_overwrite(100, [](char*, std::size_t) { return 0; });  // int result
    if (!s.empty() || s.c_str()[0] != '\0') return false;
  }
  {
    // Any integer-like result type works.
    std::string s;
    s.resize_and_overwrite(4, [](char* p, std::size_t) -> short {
      p[0] = 'x';
      p[1] = 'y';
      return 2;
    });
    if (s != "xy") return false;
  }
  {
    std::string s;
    s.resize_and_overwrite(200, [](char* p, std::size_t n) {
      for (std::size_t i = 0; i < n; ++i) p[i] = 'z';
      return n;
    });
    if (s.size() != 200 || s[199] != 'z' || s.data()[200] != '\0') return false;
  }
  {
    // The operation is invoked as an rvalue: std::move(op)(p, m).
    bool called = false;
    std::string s;
    s.resize_and_overwrite(3, MoveCalled{&called});
    if (!called || s != "mmm") return false;
  }
  {
    // m is passed as size_type.
    std::string s;
    s.resize_and_overwrite(2, [](char* p, auto m) {
      static_assert(std::is_same_v<std::remove_const_t<decltype(m)>, std::string::size_type>);
      p[0] = p[1] = 'q';
      return m;
    });
    if (s != "qq") return false;
  }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::wstring w = L"wide";
  w.resize_and_overwrite(6, [](wchar_t* p, std::size_t) {
    p[4] = L'!';
    p[5] = L'?';
    return 6;
  });
  CHECK(w == L"wide!?");
  return 0;
}
