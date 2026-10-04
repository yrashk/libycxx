// [format.functions]: format_to writes the whole output and returns out + N (/16-/17);
// format_to_n writes the first M = clamp(n, 0, N) characters and returns {out + M, N}
// (/19-/23; negative n: format_to_n_negative); formatted_size returns N (/27); the wide overloads; vformat_to.
#include <format>
#include <iterator>
#include <string>
#include "check.hpp"

int main() {
  char buf[32] = {};
  char* end = std::format_to(buf, "{}-{}", 12, "ab");
  CHECK(end == buf + 5 && std::string(buf, end) == "12-ab");
  std::string s;
  std::format_to(std::back_inserter(s), "{:>4}", 7);
  CHECK(s == "   7");
  {
    char out[8] = {'.', '.', '.', '.', '.', '.', '.', '.'};
    auto r = std::format_to_n(out, 3, "{}", 123456);
    CHECK(r.out == out + 3 && r.size == 6 && std::string(out, 4) == "123.");
    static_assert(std::is_same_v<decltype(r), std::format_to_n_result<char*>>);
  }
  {
    char out[4] = {'.', '.', '.', '.'};
    auto r = std::format_to_n(out, 0, "{}", 99);
    CHECK(r.out == out && r.size == 2 && out[0] == '.');
    r = std::format_to_n(out, 10, "{}", 99);
    CHECK(r.out == out + 2 && r.size == 2 && out[1] == '9');
  }
  CHECK(std::formatted_size("{}", 12345) == 5);
  CHECK(std::formatted_size("{:10}", 'x') == 10);
  CHECK(std::formatted_size("{:?}", "\n") == 4);
  CHECK(std::formatted_size("") == 0);
  CHECK(std::formatted_size("{:3}", "中") == 4); // code units, not columns
  wchar_t wbuf[16] = {};
  wchar_t* wend = std::format_to(wbuf, L"{}{}", L'a', 1);
  CHECK(std::wstring(wbuf, wend) == L"a1");
  CHECK(std::formatted_size(L"{:5}", 1) == 5);
  auto wr = std::format_to_n(wbuf, 1, L"{}", 42);
  CHECK(wr.size == 2 && wr.out == wbuf + 1 && wbuf[0] == L'4');
  std::string v;
  int n = 3;
  std::vformat_to(std::back_inserter(v), "{}+{}", std::make_format_args(n, n));
  CHECK(v == "3+3");
  CHECK(std::vformat("{0}{0}", std::make_format_args(n)) == "33");
  CHECK(std::vformat(L"{}", std::make_wformat_args(n)) == L"3");
}
