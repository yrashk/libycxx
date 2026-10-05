// Which std-format-spec options are valid for which argument types, checked at run time
// through vformat ([format.err.report]/1: format_error "if an argument fmt is passed that is
// not a format string for args"; [format.string.general]/5: a format-spec that does not
// conform to the format specifications for the argument type makes fmt not a format string).
//   type ([format.string.std]/1: a A b B c d e E f F g G o p P s x X ?):
//     Table 106 strings: none, s, ?          Table 107 integers: none, b, B, c, d, o, x, X
//     Table 108 charT: none, c, ?, b, B, d, o, x, X     Table 109 bool: none, s, b, B, d, o, x, X
//     Table 110 floating point: none, a, A, e, E, f, F, g, G     Table 111 pointers: none, p, P
//   /5 sign, /7 #: "only valid for arithmetic types other than charT and bool or when an
//     integer presentation type is specified";
//   /8 0: "valid for arithmetic types other than charT and bool, pointer types, or when an
//     integer presentation type is specified";
//   /15 precision: "valid for floating-point and string types";
//   /17 L: "only valid for arithmetic types" (bool and charT are arithmetic types);
//   /10 dynamic width / precision: "valid only if the corresponding formatting argument is of
//     standard signed or unsigned integer type" ([basic.fundamental]/1-2: signed char ... long
//     long and their unsigned counterparts; not char, wchar_t or bool).
// char formatted into a wide context uses formatter<char, wchar_t> ([format.formatter.spec]
// /2.1), a charT formatter for the purposes of Table 108.
// REQUIRES: exceptions
#include <format>
#include <string>
#include <string_view>
#include "check.hpp"

template <class CharT, class... Args>
bool valid(std::basic_string_view<CharT> fmt, Args... args) {
  try {
    if constexpr (std::is_same_v<CharT, char>) (void)std::vformat(fmt, std::make_format_args(args...));
    else (void)std::vformat(fmt, std::make_wformat_args(args...));
  } catch (const std::format_error&) {
    return false;
  }
  return true;
}

// For every type character, checks valid("{:<t>}", arg) against the set `ok`.
template <class CharT, class T>
bool types_match(T arg, std::string_view ok) {
  const std::string_view all = "aAbBcdeEfFgGopPsxX?";
  for (char t : all) {
    CharT f[] = {CharT('{'), CharT(':'), CharT(t), CharT('}')};
    const bool expect = ok.find(t) != std::string_view::npos;
    if (valid<CharT>(std::basic_string_view<CharT>(f, 4), arg) != expect) return false;
  }
  CharT none[] = {CharT('{'), CharT('}')};
  return valid<CharT>(std::basic_string_view<CharT>(none, 2), arg);
}

// The option `opt` (one of "+", "-", " ", "#", "0", ".3", "L") before the type `t` ("" for
// none).
template <class CharT, class T>
bool opt_valid(T arg, std::string_view opt, std::string_view t = "") {
  std::basic_string<CharT> f;
  for (char c : std::string_view("{:")) f += CharT(c);
  for (char c : opt) f += CharT(c);
  if (opt == "0") f += CharT('5');  // 0 needs no width, but give it one
  for (char c : t) f += CharT(c);
  f += CharT('}');
  return valid<CharT>(std::basic_string_view<CharT>(f), arg);
}

int main() {
  const char* cs = "s";
  std::string str = "s";
  std::string_view sv = "s";
  void* vp = nullptr;
  const void* cvp = nullptr;
  // Type characters.
  CHECK(types_match<char>(1, "bBcdoxX"));
  CHECK(types_match<char>(1ULL, "bBcdoxX"));
  CHECK(types_match<char>(static_cast<short>(1), "bBcdoxX"));
  CHECK(types_match<char>(static_cast<signed char>(1), "bBcdoxX"));
  CHECK(types_match<char>(static_cast<unsigned char>(1), "bBcdoxX"));
  CHECK(types_match<char>('c', "cbBdoxX?"));
  CHECK(types_match<char>(true, "sbBdoxX"));
  CHECK(types_match<char>(1.0, "aAeEfFgG"));
  CHECK(types_match<char>(1.0f, "aAeEfFgG"));
  CHECK(types_match<char>(1.0L, "aAeEfFgG"));
  CHECK(types_match<char>(cs, "s?"));
  CHECK(types_match<char>(str, "s?"));
  CHECK(types_match<char>(sv, "s?"));
  CHECK(types_match<char>(vp, "pP"));
  CHECK(types_match<char>(cvp, "pP"));
  CHECK(types_match<char>(nullptr, "pP"));
  CHECK(types_match<wchar_t>(L'c', "cbBdoxX?"));
  CHECK(types_match<wchar_t>('c', "cbBdoxX?"));   // formatter<char, wchar_t>
  CHECK(types_match<wchar_t>(7L, "bBcdoxX"));
  CHECK(types_match<wchar_t>(false, "sbBdoxX"));
  CHECK(types_match<wchar_t>(2.0, "aAeEfFgG"));
  CHECK(types_match<wchar_t>(std::wstring_view(L"w"), "s?"));
  CHECK(types_match<wchar_t>(nullptr, "pP"));

  // sign and #: arithmetic types other than charT and bool, or an integer presentation type.
  for (std::string_view o : {"+", "-", " ", "#"}) {
    CHECK(opt_valid<char>(1, o) && opt_valid<char>(1u, o, "x") && opt_valid<char>(1.5, o) && opt_valid<char>(1.5, o, "e"));
    CHECK(!opt_valid<char>('c', o) && !opt_valid<char>('c', o, "c") && opt_valid<char>('c', o, "d") && opt_valid<char>('c', o, "X"));
    CHECK(!opt_valid<char>(true, o) && !opt_valid<char>(true, o, "s") && opt_valid<char>(true, o, "b"));
    CHECK(!opt_valid<char>(cs, o) && !opt_valid<char>(str, o, "s") && !opt_valid<char>(sv, o, "?"));
    CHECK(!opt_valid<char>(vp, o) && !opt_valid<char>(nullptr, o, "p"));
    CHECK(!opt_valid<wchar_t>(L'c', o) && opt_valid<wchar_t>(L'c', o, "o") && !opt_valid<wchar_t>('c', o));
  }
  CHECK(opt_valid<char>(1, "+", "c") && opt_valid<char>(1, "#", "c"));  // c is in Table 107 for integers
  // 0: also pointer types.
  CHECK(opt_valid<char>(1, "0") && opt_valid<char>(1.5, "0") && opt_valid<char>(1.5, "0", "a"));
  CHECK(opt_valid<char>(vp, "0") && opt_valid<char>(cvp, "0", "P") && opt_valid<char>(nullptr, "0"));
  CHECK(!opt_valid<char>('c', "0") && opt_valid<char>('c', "0", "x") && !opt_valid<char>('c', "0", "?"));
  CHECK(!opt_valid<char>(true, "0") && opt_valid<char>(true, "0", "d"));
  CHECK(!opt_valid<char>(cs, "0") && !opt_valid<char>(sv, "0", "s"));
  // precision: floating-point and string types only.
  CHECK(opt_valid<char>(1.5, ".3") && opt_valid<char>(1.5, ".3", "g") && opt_valid<char>(1.5f, ".0", "a"));
  CHECK(opt_valid<char>(cs, ".3") && opt_valid<char>(str, ".3", "s") && opt_valid<char>(sv, ".3", "?"));
  CHECK(!opt_valid<char>(1, ".3") && !opt_valid<char>(1, ".3", "x") && !opt_valid<char>(1, ".3", "c"));
  CHECK(!opt_valid<char>('c', ".3") && !opt_valid<char>('c', ".3", "?") && !opt_valid<char>('c', ".3", "d"));
  CHECK(!opt_valid<char>(true, ".3") && !opt_valid<char>(vp, ".3") && !opt_valid<char>(nullptr, ".3", "p"));
  // L: arithmetic types, including bool and charT.
  CHECK(opt_valid<char>(1, "L") && opt_valid<char>(1, "L", "x") && opt_valid<char>(1.5, "L", "f"));
  CHECK(opt_valid<char>(true, "L") && opt_valid<char>(true, "L", "d"));
  CHECK(opt_valid<char>('c', "L") && opt_valid<char>('c', "L", "d"));
  CHECK(!opt_valid<char>(cs, "L") && !opt_valid<char>(sv, "L", "s") && !opt_valid<char>(vp, "L") && !opt_valid<char>(nullptr, "L", "p"));

  // Dynamic width and precision arguments: standard signed or unsigned integer types only.
  const auto dyn_ok = [](auto w) {
    return valid<char>("{:{}}", 1, w) && valid<char>("{:.{}}", 1.5, w) && valid<char>("{:{}.{}}", "abc", w, w);
  };
  CHECK(dyn_ok(static_cast<signed char>(2)) && dyn_ok(static_cast<unsigned char>(2)));
  CHECK(dyn_ok(static_cast<short>(2)) && dyn_ok(static_cast<unsigned short>(2)));
  CHECK(dyn_ok(2) && dyn_ok(2u) && dyn_ok(2L) && dyn_ok(2UL) && dyn_ok(2LL) && dyn_ok(2ULL));
  CHECK(!valid<char>("{:{}}", 1, 'c') && !valid<char>("{:.{}}", 1.5, true) && !valid<char>("{:{}}", 1, 2.0));
  CHECK(!valid<char>("{:{}}", 1, cs) && !valid<char>("{:{}}", 1, nullptr));
  CHECK(!valid<wchar_t>(L"{:{}}", 1, L'c') && !valid<wchar_t>(L"{:{}}", 1, 'c'));
  CHECK(valid<wchar_t>(L"{:{}}", 1, static_cast<unsigned char>(3)));
  // Negative values are format errors (/10).
  CHECK(!valid<char>("{:{}}", 1, static_cast<signed char>(-1)) && !valid<char>("{:.{}}", "a", -1LL));
  CHECK(valid<char>("{:.{}}", "a", 0) && valid<char>("{:{}}", 1, 0));

  // Grammar order: fill-and-align sign # 0 width precision L type, nothing else.
  CHECK(valid<char>("{:*^+#010.3Lf}", 1.5));
  CHECK(valid<char>("{:*>+#012Lx}", 255));
  CHECK(!valid<char>("{:+*^10}", 1));   // sign before fill-and-align
  CHECK(!valid<char>("{:#+}", 1));      // # before sign
  CHECK(!valid<char>("{:0#}", 1));      // 0 before #
  CHECK(!valid<char>("{:L5}", 1));      // L before width
  CHECK(!valid<char>("{:xL}", 1));      // type before L
  CHECK(!valid<char>("{:.3.3}", 1.5));
  CHECK(!valid<char>("{:5.}", 1.5));    // precision needs digits or a nested field
  CHECK(!valid<char>("{:++}", 1));
  CHECK(!valid<char>("{:xx}", 1));
  CHECK(!valid<char>("{:{<5}", 1));     // fill cannot be {
  CHECK(!valid<char>("{:é<5}", 1.5));  // fill is one Unicode scalar value (/3)
  return 0;
}
