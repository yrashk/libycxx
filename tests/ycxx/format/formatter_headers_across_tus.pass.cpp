// [format.formatter.spec]/2: "Each header that declares the template formatter provides the
// following enabled specializations" (characters, strings, arithmetic types, pointers);
// [vector.syn] and [vector.bool.fmt] (formatter<vector<bool>::reference>), [stack.syn] and
// [queue.syn] ([container.adaptors.format]) declare formatter. [basic.def.odr]/15: a class
// template specialization used in several translation units is one entity with one
// definition, whichever of these headers and <format> each translation unit includes, and in
// whichever order.
// Six translation units: this one (<format> first), tu_b (<vector>, <format>, <queue>,
// <stack>), tu_c (<vector> with a program-defined formatter written before <format>, <stack>,
// <queue>, <format> last), tu_d (<queue>, <format>, <stack>, <vector>), tu_e (<vector> only)
// and tu_f (<stack> and <queue> only). The formatting translation units produce the same text,
// equal to the draft's results; every translation unit agrees on the size and the identity
// (typeid) of the formatter specializations; formatter objects made in tu_e (without <format>)
// format correctly here.
// FILES: ../support/fmt_tu/tu_b.cpp ../support/fmt_tu/tu_c.cpp ../support/fmt_tu/tu_d.cpp
// FILES: ../support/fmt_tu/tu_e.cpp ../support/fmt_tu/tu_f.cpp
#include <format>
#include <queue>
#include <stack>
#include <string>
#include <vector>
#include "../support/fmt_tu/flag.hpp"
#include "../support/fmt_tu/body.hpp"
#include "../support/fmt_tu/facts.hpp"
#include "check.hpp"

// formats through formatter objects obtained from tu_e (not usable in constant expressions,
// so the format strings are checked at run time by vformat)
struct ViaInt {
  int i;
};
template <>
struct std::formatter<ViaInt> {
  std::formatter<int> f = tu_e_int_formatter();
  constexpr auto parse(std::format_parse_context& pc) { return f.parse(pc); }
  auto format(const ViaInt& x, std::format_context& ctx) const { return f.format(x.i, ctx); }
};
struct ViaRef {
  std::vector<bool>::reference r;
};
template <>
struct std::formatter<ViaRef> {
  std::formatter<std::vector<bool>::reference> f = tu_e_ref_formatter();
  constexpr auto parse(std::format_parse_context& pc) { return f.parse(pc); }
  auto format(const ViaRef& x, std::format_context& ctx) const { return f.format(x.r, ctx); }
};

static bool same(const Facts& a, const Facts& b, bool with_ref) {
  bool ok = a.size_int == b.size_int && a.size_double == b.size_double && a.size_cstr == b.size_cstr &&
            a.size_string == b.size_string && a.size_char == b.size_char && a.size_wchar == b.size_wchar &&
            a.size_ptr == b.size_ptr && *a.ti_int == *b.ti_int && *a.ti_string == *b.ti_string &&
            *a.ti_wchar == *b.ti_wchar;
  if (with_ref) ok = ok && a.size_ref == b.size_ref && a.ti_ref && b.ti_ref && *a.ti_ref == *b.ti_ref;
  return ok;
}

int main() {
  const std::string want =
      "[1, 2, 3]|1, 2, 3|[01, 02, 03]|[true, false, true]|1|0x1|*false*|[3, 1, 2]|[7, 8]|9|ab|['a', 'b']|"
      "[\"x\", \"y\"]|[[1], [2, 3]]|[false, true]|true|42|-7|3.500000e+00|str|s2|'c'";
  const std::wstring wwant = L"[1, 2, 3]|[true, false]|0|[4]|w|  12";
  CHECK(format_all() == want);
  CHECK(tu_b_text() == want);
  CHECK(tu_c_text() == want);
  CHECK(tu_d_text() == want);
  CHECK(wformat_all() == wwant);
  CHECK(tu_b_wtext() == wwant && tu_c_wtext() == wwant && tu_d_wtext() == wwant);

  const Facts here = facts_here<std::vector<bool>::reference>();
  CHECK(same(here, tu_b_facts(), true));
  CHECK(same(here, tu_c_facts(), true));
  CHECK(same(here, tu_d_facts(), true));
  CHECK(same(here, tu_e_facts(), true));
  CHECK(same(here, tu_f_facts(), false));
  CHECK(tu_f_facts().ti_ref == nullptr);

  std::vector<bool> vb{true, false};
  ViaInt a{42}, b{255}, c{1};
  CHECK(std::vformat("{}|{:+06}|{:#x}|{:*^5}", std::make_format_args(a, a, b, c)) == "42|+00042|0xff|**1**");
  ViaRef t{vb[0]}, f{vb[1]};
  CHECK(std::vformat("{}|{:d}|{:>6}", std::make_format_args(t, f, f)) == "true|0| false");
}
