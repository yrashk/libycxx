// std::regex_traits' character classes. [re.traits]/9: lookup_classname returns an
// (unspecified) value naming the classification; "The value returned shall be independent of
// the case of the characters in the sequence"; with icase the mask ignores the case of the
// characters matched (footnote: "lower" and "upper" then mean the same as "alpha"); an
// unrecognized name gives char_class_type(). /10: regex_traits<char> and <wchar_t> recognize at
// least the names of Table 121 (alnum alpha blank cntrl digit d graph lower print punct space s
// upper w xdigit). /11-12: isctype(c, f) is ctype::is(convert(f), c), and '_' is also a member
// of any f that contains "w"; Example 1 (d | upper) and Example 2 (w: 'A' and '_' yes, ' ' no).
// /3-5: length is char_traits::length, translate(c) is c, translate_nocase(c) is tolower.
// /14: value(ch, radix) is the digit's value in radix 8, 10 or 16, else -1. /6: transform is
// the collate facet's.
// COUNTERPART: libcxx:re/re.traits/lookup_classname.pass.cpp
// COUNTERPART: libstdcxx:28_regex/traits/wchar_t/lookup_classname.cc
#include <cstring>
#include <locale>
#include <regex>
#include <string>
#include "check.hpp"

template <class C>
struct names;
template <>
struct names<char> {
  static constexpr const char* all[] = {"alnum", "alpha", "blank", "cntrl", "digit", "d", "graph", "lower",
                                        "print", "punct", "space", "s", "upper", "w", "xdigit"};
};
template <>
struct names<wchar_t> {
  static constexpr const wchar_t* all[] = {L"alnum", L"alpha", L"blank", L"cntrl", L"digit", L"d", L"graph", L"lower",
                                           L"print", L"punct", L"space", L"s", L"upper", L"w", L"xdigit"};
};

template <class C>
typename std::regex_traits<C>::char_class_type cls(const std::regex_traits<C>& t, const C* n, bool icase = false) {
  std::basic_string<C> s(n);
  return t.lookup_classname(s.begin(), s.end(), icase);
}

template <class C>
void run(const C* upper_name, const C* bad, const C* Digit) {
  using T = std::regex_traits<C>;
  using M = typename T::char_class_type;
  T t;
  // Every name of Table 121 is recognized, in any case.
  for (const C* n : names<C>::all) {
    CHECK(cls(t, n) != M());
    std::basic_string<C> up(n);
    for (C& c : up) c = std::use_facet<std::ctype<C>>(t.getloc()).toupper(c);
    CHECK(t.lookup_classname(up.begin(), up.end()) == cls(t, n));
  }
  CHECK(cls(t, bad) == M());
  CHECK(cls(t, Digit) == cls(t, names<C>::all[4]));  // "Digit" == "digit"

  const auto& ct = std::use_facet<std::ctype<C>>(t.getloc());
  auto w = [&](char c) { return ct.widen(c); };
  // isctype against each class, for a few characters of the "C" locale.
  struct row { const char* name; const char* in; const char* out; };
  const row rows[] = {{"alnum", "a0Z", " _-"}, {"alpha", "aZ", "0 _"},  {"blank", " \t", "a\n"},
                      {"cntrl", "\n\t", "a "},  {"digit", "09", "aX"},   {"d", "05", "x "},
                      {"graph", "a!", " \n"},   {"lower", "az", "AZ0"},  {"print", "a ", "\n"},
                      {"punct", "!.", "a "},    {"space", " \n\t", "a"}, {"s", " \v", "_"},
                      {"upper", "AZ", "az"},    {"w", "aZ0_", " -."},   {"xdigit", "09afAF", "gG "}};
  for (const row& r : rows) {
    std::basic_string<C> n;
    for (const char* p = r.name; *p; ++p) n += w(*p);
    const M f = t.lookup_classname(n.begin(), n.end());
    for (const char* p = r.in; *p; ++p) CHECK(t.isctype(w(*p), f));
    for (const char* p = r.out; *p; ++p) CHECK(!t.isctype(w(*p), f));
  }
  // icase: "lower" and "upper" then match letters of either case.
  const M lo = cls(t, names<C>::all[7], true), up = cls(t, upper_name, true);
  CHECK(t.isctype(w('A'), lo) && t.isctype(w('a'), lo) && t.isctype(w('a'), up) && t.isctype(w('A'), up));
  CHECK(!t.isctype(w('0'), lo));
  // Example 1: masks combine with |.
  M f = cls(t, names<C>::all[5]);
  f |= cls(t, upper_name);
  CHECK(t.isctype(w('7'), f) && t.isctype(w('Q'), f) && !t.isctype(w('q'), f));
  // Example 2.
  const M wm = cls(t, names<C>::all[13]);
  CHECK(t.isctype(w('A'), wm) && t.isctype(w('_'), wm) && !t.isctype(w(' '), wm));

  // length, translate, translate_nocase, value.
  const C hello[] = {w('h'), w('e'), w('l'), w('l'), w('o'), C()};
  CHECK(T::length(hello) == 5);
  CHECK(t.translate(w('Q')) == w('Q'));
  CHECK(t.translate_nocase(w('Q')) == w('q') && t.translate_nocase(w('q')) == w('q'));
  CHECK(t.value(w('7'), 8) == 7 && t.value(w('8'), 8) == -1);
  CHECK(t.value(w('9'), 10) == 9 && t.value(w('a'), 10) == -1);
  CHECK(t.value(w('f'), 16) == 15 && t.value(w('F'), 16) == 15 && t.value(w('g'), 16) == -1);

  // /6: transform is the collate facet's transform (transform_primary: its own test).
  const std::basic_string<C> ab{w('a'), w('b')}, ac{w('a'), w('c')};
  const auto& col = std::use_facet<std::collate<C>>(t.getloc());
  CHECK(t.transform(ab.begin(), ab.end()) == col.transform(ab.data(), ab.data() + ab.size()));
  CHECK(t.transform(ab.begin(), ab.end()) < t.transform(ac.begin(), ac.end()));
}

int main() {
  run<char>("upper", "nosuchclass", "Digit");
  run<wchar_t>(L"upper", L"nosuchclass", L"Digit");
}
