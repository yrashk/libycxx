// [ispanstream], [ospanstream], [spanstream]: basic_ispanstream reads from a span (also
// constructible from a read-only range of char: "template<class ROS> explicit
// basic_ispanstream(ROS&& s)" for a borrowed range convertible to span<const charT>);
// basic_ospanstream writes into a span and sets badbit when it is full; span() / span(s);
// basic_spanstream does both.
#include <spanstream>
#include <span>
#include <string_view>
#include "check.hpp"

int main() {
  char data[] = "12 34 word";
  std::ispanstream is(std::span<char>(data, sizeof data - 1));
  int a, b;
  std::string_view sv;
  is >> a >> b;
  CHECK(a == 12 && b == 34);
  CHECK(is.span().size() == sizeof data - 1);

  const char ro[] = "7 8";
  std::ispanstream cis(std::span<const char>(ro, 3));
  cis >> a >> b;
  CHECK(a == 7 && b == 8);
  CHECK(cis.eof());
  std::string_view view = "99";
  std::ispanstream from_view(view);  // a borrowed contiguous range of const char
  from_view >> a;
  CHECK(a == 99);

  char out[8] = {};
  std::ospanstream os(std::span<char>(out, 8));
  os << 123 << '-' << 45;
  CHECK(std::string_view(os.span().data(), os.span().size()) == "123-45");
  CHECK(os.good());
  os << "long";  // only 2 of 4 fit
  CHECK(os.bad());
  CHECK(std::string_view(out, 8) == "123-45lo");

  char both[16] = {};
  std::spanstream ss(std::span<char>(both, 16));
  ss << 5 << ' ' << 6 << ' ';
  int x = 0, y = 0;
  ss >> x >> y;
  CHECK(x == 5 && y == 6);
  ss.span(std::span<char>(both, 4));
  CHECK(ss.span().size() == 0);
  return 0;
}
