// [format.arg]/6: basic_format_arg stores bool and char-type as is; char as wchar_t for a wide
// context (6.2); signed / unsigned integers no wider than int as int / unsigned int (6.3-6.4),
// otherwise as long long / unsigned long long (6.5-6.6); standard floating-point types as is
// (6.7); basic_string / basic_string_view as basic_string_view<char-type> (6.8); char-type
// pointers as const char-type* (6.9); void pointers and nullptr_t as const void* (6.10); other
// types as handle (6.11). /3, /7: a default-constructed basic_format_arg is false (monostate).
// /8-9 (C++26): the visit members. [format.args]/4: get(i) returns a default-constructed
// basic_format_arg when i >= size. [format.arg.store]/3-4: make_format_args /
// make_wformat_args. /14: handle::format(parse_ctx, format_ctx) calls the formatter's parse and
// format.
// COUNTERPART: libcxx:utilities/format/format.formatter/format.formatter.spec/formatter.handle.pass.cpp
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include "check.hpp"

template <class Expected>
struct Is {
  template <class T>
  bool operator()(const T&) const {
    return std::is_same_v<T, Expected>;
  }
};

struct Point {
  int x, y;
};
template <>
struct std::formatter<Point> {
  bool swap = false;
  constexpr auto parse(std::format_parse_context& pc) {
    auto it = pc.begin();
    if (it != pc.end() && *it == 's') {
      swap = true;
      ++it;
    }
    return it;
  }
  auto format(const Point& p, std::format_context& fc) const {
    return std::format_to(fc.out(), "({},{})", swap ? p.y : p.x, swap ? p.x : p.y);
  }
};

int main() {
  using Arg = std::basic_format_arg<std::format_context>;
  Arg empty;
  CHECK(!empty);
  CHECK(empty.visit(Is<std::monostate>{}));

  bool b = true;
  char c = 'x';
  signed char sc = -1;
  unsigned char uc = 200;
  short s = -3;
  unsigned short us = 3;
  int i = -4;
  unsigned u = 4;
  long l = -5;
  unsigned long ul = 5;
  long long ll = -6;
  unsigned long long ull = 6;
  float f = 1.5f;
  double d = 2.5;
  long double ld = 3.5L;
  const char* cs = "cs";
  char arr[] = "arr";
  std::string str = "str";
  std::string_view sv = "sv";
  void* vp = &i;
  const void* cvp = &i;
  std::nullptr_t np = nullptr;
  Point pt{1, 2};
  auto store = std::make_format_args(b, c, sc, uc, s, us, i, u, l, ul, ll, ull, f, d, ld, cs, arr, str,
                                     sv, vp, cvp, np, pt);
  std::format_args args = store;
  int k = 0;
  CHECK(args.get(k++).visit(Is<bool>{}));
  CHECK(args.get(k++).visit(Is<char>{}));
  CHECK(args.get(k++).visit(Is<int>{}));
  CHECK(args.get(k++).visit(Is<unsigned>{}));
  CHECK(args.get(k++).visit(Is<int>{}));
  CHECK(args.get(k++).visit(Is<unsigned>{}));
  CHECK(args.get(k++).visit(Is<int>{}));
  CHECK(args.get(k++).visit(Is<unsigned>{}));
  CHECK(args.get(k++).visit(Is<std::conditional_t<sizeof(long) <= sizeof(int), int, long long>>{}));
  CHECK(args.get(k++).visit(
      Is<std::conditional_t<sizeof(long) <= sizeof(int), unsigned, unsigned long long>>{}));
  CHECK(args.get(k++).visit(Is<long long>{}));
  CHECK(args.get(k++).visit(Is<unsigned long long>{}));
  CHECK(args.get(k++).visit(Is<float>{}));
  CHECK(args.get(k++).visit(Is<double>{}));
  CHECK(args.get(k++).visit(Is<long double>{}));
  CHECK(args.get(k++).visit(Is<const char*>{}));
  CHECK(args.get(k++).visit(Is<const char*>{}));
  CHECK(args.get(k++).visit(Is<std::string_view>{}));
  CHECK(args.get(k++).visit(Is<std::string_view>{}));
  CHECK(args.get(k++).visit(Is<const void*>{}));
  CHECK(args.get(k++).visit(Is<const void*>{}));
  CHECK(args.get(k++).visit(Is<const void*>{}));
  CHECK(args.get(k++).visit(Is<Arg::handle>{}));
  CHECK(k == 23);
  CHECK(!args.get(23) && !args.get(1000));

  // values
  CHECK(args.get(2).visit([](auto v) { if constexpr (std::is_same_v<decltype(v), int>) return v == -1; else return false; }));
  CHECK(args.get(3).visit([](auto v) { if constexpr (std::is_same_v<decltype(v), unsigned>) return v == 200u; else return false; }));
  CHECK(args.get(17).visit([](auto v) { if constexpr (std::is_same_v<decltype(v), std::string_view>) return v == "str"; else return false; }));
  // visit<R>
  long r = args.get(6).visit<long>([](auto v) -> long {
    if constexpr (std::is_arithmetic_v<decltype(v)>) return static_cast<long>(v) * 10;
    else return 0;
  });
  CHECK(r == -40);

  // a wide context stores char as wchar_t
  using WArg = std::basic_format_arg<std::wformat_context>;
  char nc = 'n';
  wchar_t wc = L'w';
  std::wstring ws = L"ws";
  auto wstore = std::make_wformat_args(nc, wc, ws);
  std::wformat_args wargs = wstore;
  CHECK(wargs.get(0).visit(Is<wchar_t>{}));
  CHECK(wargs.get(0).visit([](auto v) { if constexpr (std::is_same_v<decltype(v), wchar_t>) return v == L'n'; else return false; }));
  CHECK(wargs.get(1).visit(Is<wchar_t>{}));
  CHECK(wargs.get(2).visit(Is<std::wstring_view>{}));
  CHECK(!WArg());

  // the handle formats through the user's formatter, parsing the remaining spec
  CHECK(std::vformat("{}", std::make_format_args(pt)) == "(1,2)");
  CHECK(std::format("{:s}|{}", pt, pt) == "(2,1)|(1,2)");
  return 0;
}
