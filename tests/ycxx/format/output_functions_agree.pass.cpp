// [format.functions]: all formatting functions produce the same character representation.
// /3-/4: format(fmt, args...) is vformat(fmt.str, make_format_args(args...)); /5-/7: the
// locale overloads (loc only matters for L); /9-/17: format_to / vformat_to place the N
// characters into [out, out + N) and return out + N, for Out = back_insert_iterator<string>
// (appending to what the string already holds), a raw pointer, and the wide forms; /19-/23:
// format_to_n places the first M = clamp(n, 0, N) characters and returns {out + M, N} with
// N = formatted_size(fmt, args...); /25-/27: formatted_size returns N. [format.fmt.string]
// /2-/3: basic_format_string's get() returns the string the object was constructed from.
#include <algorithm>
#include <format>
#include <iterator>
#include <locale>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#include "check.hpp"

template <class... Args>
bool agree(std::string_view expect, std::format_string<Args...> fmt, Args&&... args) {
  if (fmt.get().data() == nullptr) return false;
  const std::string s = std::format(fmt, std::forward<Args>(args)...);
  if (s != expect) return false;
  if (std::vformat(fmt.get(), std::make_format_args(args...)) != s) return false;
  if (std::format(std::locale::classic(), fmt, std::forward<Args>(args)...) != s) return false;
  if (std::formatted_size(fmt, std::forward<Args>(args)...) != s.size()) return false;
  if (std::formatted_size(std::locale::classic(), fmt, std::forward<Args>(args)...) != s.size()) return false;
  // back_insert_iterator<string>: appends; the returned iterator still appends.
  std::string app = "pre:";
  std::back_insert_iterator<std::string> bi = std::format_to(std::back_inserter(app), fmt, std::forward<Args>(args)...);
  *bi = '!';
  if (app != "pre:" + s + "!") return false;
  std::string vapp;
  std::vformat_to(std::back_inserter(vapp), fmt.get(), std::make_format_args(args...));
  if (vapp != s) return false;
  // raw pointer: exactly N characters, nothing after them touched.
  std::vector<char> buf(s.size() + 4, '#');
  char* end = std::format_to(buf.data(), fmt, std::forward<Args>(args)...);
  if (end != buf.data() + s.size() || std::string(buf.data(), end) != s || buf[s.size()] != '#') return false;
  // format_to_n with every n from 0 to N + 1 (negative n: format_to_n_negative.pass.cpp).
  for (std::ptrdiff_t n = 0; n <= static_cast<std::ptrdiff_t>(s.size()) + 1; ++n) {
    std::vector<char> b(s.size() + 4, '#');
    auto r = std::format_to_n(b.data(), n, fmt, std::forward<Args>(args)...);
    const std::size_t m = std::min(static_cast<std::size_t>(n), s.size());
    if (r.out != b.data() + m || r.size != static_cast<std::ptrdiff_t>(s.size())) return false;
    if (std::string(b.data(), m) != s.substr(0, m) || b[m] != '#') return false;
  }
  std::string tn;
  auto rn = std::format_to_n(std::back_inserter(tn), 3, fmt, std::forward<Args>(args)...);
  return tn == s.substr(0, 3) && rn.size == static_cast<std::ptrdiff_t>(s.size());
}

template <class... Args>
bool wagree(std::wstring_view expect, std::wformat_string<Args...> fmt, Args&&... args) {
  const std::wstring s = std::format(fmt, std::forward<Args>(args)...);
  if (s != expect) return false;
  if (std::vformat(fmt.get(), std::make_wformat_args(args...)) != s) return false;
  if (std::formatted_size(fmt, std::forward<Args>(args)...) != s.size()) return false;
  std::wstring app = L">";
  std::format_to(std::back_inserter(app), fmt, std::forward<Args>(args)...);
  if (app != L">" + s) return false;
  std::vector<wchar_t> buf(s.size() + 1, L'#');
  wchar_t* end = std::format_to(buf.data(), fmt, std::forward<Args>(args)...);
  if (end != buf.data() + s.size() || std::wstring(buf.data(), end) != s) return false;
  auto r = std::format_to_n(buf.data(), 1, fmt, std::forward<Args>(args)...);
  return r.size == static_cast<std::ptrdiff_t>(s.size()) && r.out == buf.data() + (s.empty() ? 0 : 1);
}

int main() {
  int i = 42;
  const char* cs = "str";
  std::vector<int> v = {1, 2, 3};
  CHECK(agree("", ""));
  CHECK(agree("plain {text}", "plain {{text}}"));
  CHECK(agree("42", "{}", i));
  CHECK(agree("   42|42   ", "{:5}|{:<5}", i, i));
  CHECK(agree("0x2a 0b101010 052", "{0:#x} {0:#b} {0:#o}", i));
  CHECK(agree("-1.50e+00", "{:.2e}", -1.5));
  CHECK(agree("str|st", "{}|{:.2}", cs, cs));
  CHECK(agree("\"a\\n\"", "{:?}", std::string("a\n")));
  CHECK(agree("中中x", "{:中>3}", 'x'));
  CHECK(agree("[1, 2, 3]", "{}", v));
  CHECK(agree("(1, \"two\")", "{}", std::pair(1, std::string("two"))));
  CHECK(agree("true 0x0", "{} {}", true, nullptr));
  CHECK(agree("\U0001F921\U0001F921", "{}{}", "\U0001F921", std::string_view("\U0001F921")));
  CHECK(agree(std::string(300, '-'), "{:->300}", ""));
  CHECK(wagree(L"", L""));
  CHECK(wagree(L"42 x", L"{} {}", i, 'x'));
  CHECK(wagree(L"[1, 2, 3]", L"{}", v));
  CHECK(wagree(L"  ab", L"{:>4}", L"ab"));
  // basic_format_string::get() returns the original string, for format_string built from a
  // string literal and from a string_view constant (runtime_format: runtime_format.pass.cpp).
  std::format_string<int> f1 = "<{}>";
  static constexpr std::string_view sv = "<{}>";
  std::format_string<int> f2 = sv;
  CHECK(f1.get() == "<{}>" && f2.get() == "<{}>" && f2.get().data() == sv.data());
  CHECK(std::format(f2, 1) == "<1>");
  std::wformat_string<int> wf = L"{}";
  CHECK(wf.get() == L"{}");
  return 0;
}
