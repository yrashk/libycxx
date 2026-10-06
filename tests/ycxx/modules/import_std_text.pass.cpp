// [std.modules]/2: `import std;` provides strings, formatting and printing, iostreams, locale,
// regex, charconv and text_encoding, including the standard stream objects.
// MODULES: std
import std;
#include "module_check.hpp"

struct point {
  int x, y;
};
template <>
struct std::formatter<point> : std::formatter<int> {
  auto format(const point& p, std::format_context& ctx) const {
    return std::format_to(ctx.out(), "({}, {})", p.x, p.y);
  }
};

int main() {
  std::string s = "hello";
  s += ' ';
  s.append("world");
  CHECK(s.size() == 11 && s.starts_with("hello") && s.find("world") == 6);
  std::string_view sv = s;
  CHECK(sv.substr(6) == "world");
  std::wstring ws = L"w";
  std::u8string u8 = u8"x";
  CHECK(ws.size() == 1 && u8.size() == 1);
  CHECK(std::to_string(42) == "42" && std::stoi("17") == 17);
  CHECK(std::format("{:>5}|{:x}|{}", 7, 255, point{1, 2}) == "    7|ff|(1, 2)");
  CHECK(std::format("{}", std::vector<int>{1, 2}) == "[1, 2]");
  CHECK(std::formatted_size("{}", 1234) == 4);
  std::ostringstream os;
  os << std::setw(4) << std::setfill('0') << 42 << ' ' << std::hex << 255;
  CHECK(os.str() == "0042 ff");
  std::istringstream is("12 abc");
  int i = 0;
  std::string w;
  is >> i >> w;
  CHECK(i == 12 && w == "abc");
  std::println(std::cout, "std::cout through the module: {}", 1);
  std::cout << "and operator<<" << std::endl;
  std::print("{}\n", "std::print");
  std::regex re("([a-z]+)([0-9]+)");
  std::smatch mt;
  std::string subject = "abc123";
  CHECK(std::regex_match(subject, mt, re) && mt[2] == "123");
  CHECK(std::regex_replace(std::string("a1b2"), std::regex("[0-9]"), "#") == "a#b#");
  char buf[16];
  auto r = std::to_chars(buf, buf + sizeof buf, 3.5);
  CHECK(r.ec == std::errc() && std::string_view(buf, r.ptr) == "3.5");
  double d = 0;
  std::from_chars(buf, r.ptr, d);
  CHECK(d == 3.5);
  CHECK(std::use_facet<std::ctype<char>>(std::locale::classic()).toupper('a') == 'A');
  CHECK(std::isdigit('7', std::locale()));
  CHECK(std::text_encoding::literal().mib() != std::text_encoding::id::unknown);
  std::array<char, 8> arr{};
  std::ospanstream sps(arr);
  sps << 12;
  CHECK(std::string_view(sps.span().data(), sps.span().size()) == "12");
  std::osyncstream(std::cout) << "osyncstream\n";
  std::wostringstream wos;
  wos << L"wide";
  CHECK(wos.str() == L"wide");
  return 0;
}
