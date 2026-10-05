// [locale.facet]/5: "For some standard facets a standard "..._byname" class, derived from it,
// implements the virtual function semantics equivalent to that facet of the locale constructed
// by locale(const char*) with the same name. Each such facet provides a constructor that takes a
// const char* argument, which names the locale, and a refs argument ... [and] a constructor
// that takes a string argument str and a refs argument, which has the same effect as calling
// the first constructor with the two arguments str.c_str() and refs."
// The _byname facets: ctype_byname ([locale.ctype.byname]), codecvt_byname, numpunct_byname
// ([locale.numpunct.byname]), collate_byname ([locale.collate.byname]), time_get_byname,
// time_put_byname, moneypunct_byname ([locale.moneypunct.byname]), messages_byname.
// For each locale name that locale(const char*) accepts here ("C" and "POSIX" always,
// [locale.cons]/"C"; "C.UTF-8" when the environment has it), each byname facet, installed in a
// locale or called directly, gives the results of the named locale's facet. Facet destructors
// are protected, so the facets are owned by locales (refs = 0).
// REQUIRES: exceptions
#include <cstring>
#include <ctime>
#include <iterator>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "check.hpp"

template <class F>
const F& byname_in(std::locale& holder, F* f) {
  holder = std::locale(std::locale::classic(), f);
  return std::use_facet<F>(holder);
}

template <class charT>
void check_ctype(const char* name, const std::locale& named) {
  std::locale h;
  const auto& b = byname_in(h, new std::ctype_byname<charT>(name));
  const auto& n = std::use_facet<std::ctype<charT>>(named);
  for (int i = 0; i < 128; ++i) {
    const charT c = static_cast<charT>(i);
    for (auto m : {std::ctype_base::space, std::ctype_base::print, std::ctype_base::cntrl, std::ctype_base::upper,
                   std::ctype_base::lower, std::ctype_base::alpha, std::ctype_base::digit, std::ctype_base::punct,
                   std::ctype_base::xdigit, std::ctype_base::blank, std::ctype_base::alnum, std::ctype_base::graph})
      CHECK(b.is(m, c) == n.is(m, c));
    CHECK(b.toupper(c) == n.toupper(c) && b.tolower(c) == n.tolower(c));
    CHECK(b.widen(static_cast<char>(i)) == n.widen(static_cast<char>(i)));
    CHECK(b.narrow(c, '?') == n.narrow(c, '?'));
  }
}

template <class charT>
void check_numpunct(const char* name, const std::locale& named) {
  std::locale h;
  const auto& b = byname_in(h, new std::numpunct_byname<charT>(std::string(name)));
  const auto& n = std::use_facet<std::numpunct<charT>>(named);
  CHECK(b.decimal_point() == n.decimal_point() && b.thousands_sep() == n.thousands_sep());
  CHECK(b.grouping() == n.grouping() && b.truename() == n.truename() && b.falsename() == n.falsename());
}

template <class charT>
void check_collate(const char* name, const std::locale& named) {
  std::locale h;
  const auto& b = byname_in(h, new std::collate_byname<charT>(name));
  const auto& n = std::use_facet<std::collate<charT>>(named);
  const std::basic_string<charT> words[] = {{charT('a')}, {charT('B')}, {charT('b'), charT('a')}, {charT('A')},
                                            {charT('1'), charT('0')}, {}, {charT('z'), charT(' ')}};
  for (const auto& x : words) {
    CHECK(b.transform(x.data(), x.data() + x.size()) == n.transform(x.data(), x.data() + x.size()));
    CHECK(b.hash(x.data(), x.data() + x.size()) == n.hash(x.data(), x.data() + x.size()));
    for (const auto& y : words)
      CHECK(b.compare(x.data(), x.data() + x.size(), y.data(), y.data() + y.size()) ==
            n.compare(x.data(), x.data() + x.size(), y.data(), y.data() + y.size()));
  }
}

template <class charT, bool Intl>
void check_moneypunct(const char* name, const std::locale& named) {
  std::locale h;
  const auto& b = byname_in(h, new std::moneypunct_byname<charT, Intl>(name));
  const auto& n = std::use_facet<std::moneypunct<charT, Intl>>(named);
  CHECK(b.decimal_point() == n.decimal_point() && b.thousands_sep() == n.thousands_sep());
  CHECK(b.grouping() == n.grouping() && b.curr_symbol() == n.curr_symbol());
  CHECK(b.positive_sign() == n.positive_sign() && b.negative_sign() == n.negative_sign());
  CHECK(b.frac_digits() == n.frac_digits());
  CHECK(std::memcmp(b.pos_format().field, n.pos_format().field, 4) == 0);
  CHECK(std::memcmp(b.neg_format().field, n.neg_format().field, 4) == 0);
}

template <class charT>
void check_time(const char* name, const std::locale& named) {
  std::tm t{};
  t.tm_year = 124;
  t.tm_mon = 1;
  t.tm_mday = 29;
  t.tm_hour = 13;
  t.tm_min = 5;
  t.tm_sec = 9;
  t.tm_wday = 4;
  t.tm_yday = 59;
  const charT fmt[] = {charT('%'), charT('c'), charT(' '), charT('%'), charT('x'), charT(' '), charT('%'),
                       charT('X'), charT(' '), charT('%'), charT('A'), charT(' '), charT('%'), charT('b')};
  auto put = [&](const std::locale& loc, const std::time_put<charT>& f) {
    std::basic_ostringstream<charT> os;
    os.imbue(loc);
    f.put(std::ostreambuf_iterator<charT>(os), os, charT(' '), &t, std::begin(fmt), std::end(fmt));
    return os.str();
  };
  std::locale h;
  const auto& b = byname_in(h, new std::time_put_byname<charT>(name));
  CHECK(put(h, b) == put(named, std::use_facet<std::time_put<charT>>(named)));

  std::locale g;
  const auto& gb = byname_in(g, new std::time_get_byname<charT>(name));
  const auto& gn = std::use_facet<std::time_get<charT>>(named);
  CHECK(gb.date_order() == gn.date_order());
  auto get = [&](const std::locale& loc, const std::time_get<charT>& f, const std::basic_string<charT>& s) {
    std::basic_istringstream<charT> is(s);
    is.imbue(loc);
    std::ios_base::iostate err = std::ios_base::goodbit;
    std::tm r{};
    f.get_monthname(std::istreambuf_iterator<charT>(is), std::istreambuf_iterator<charT>(), is, err, &r);
    return r.tm_mon * 10 + static_cast<int>(err == std::ios_base::goodbit || err == std::ios_base::eofbit);
  };
  const std::basic_string<charT> feb = {charT('F'), charT('e'), charT('b')};
  CHECK(get(g, gb, feb) == get(named, gn, feb));
}

template <class charT>
void check_messages(const char* name, const std::locale& named) {
  std::locale h;
  const auto& b = byname_in(h, new std::messages_byname<charT>(name));
  const auto& n = std::use_facet<std::messages<charT>>(named);
  // A catalog that does not exist: open returns a negative value for both.
  CHECK((b.open("ycxx-no-such-catalog", h) < 0) == (n.open("ycxx-no-such-catalog", named) < 0));
}

void check_codecvt(const char* name, const std::locale& named) {
  using CV = std::codecvt<wchar_t, char, std::mbstate_t>;
  std::locale h;
  const auto& b = byname_in(h, new std::codecvt_byname<wchar_t, char, std::mbstate_t>(name));
  const auto& n = std::use_facet<CV>(named);
  CHECK(b.encoding() == n.encoding() && b.max_length() == n.max_length() && b.always_noconv() == n.always_noconv());
  const char src[] = "plain ASCII 123";
  wchar_t out1[32], out2[32];
  std::mbstate_t s1{}, s2{};
  const char* e1 = nullptr;
  const char* e2 = nullptr;
  wchar_t* o1 = nullptr;
  wchar_t* o2 = nullptr;
  auto r1 = b.in(s1, src, src + sizeof src - 1, e1, out1, out1 + 32, o1);
  auto r2 = n.in(s2, src, src + sizeof src - 1, e2, out2, out2 + 32, o2);
  CHECK(r1 == r2 && e1 - src == e2 - src && o1 - out1 == o2 - out2);
  CHECK(std::wstring(out1, o1) == std::wstring(out2, o2));
}

void check_all(const char* name) {
  const std::locale named(name);
  check_ctype<char>(name, named);
  check_ctype<wchar_t>(name, named);
  check_numpunct<char>(name, named);
  check_numpunct<wchar_t>(name, named);
  check_collate<char>(name, named);
  check_collate<wchar_t>(name, named);
  check_moneypunct<char, false>(name, named);
  check_moneypunct<char, true>(name, named);
  check_moneypunct<wchar_t, false>(name, named);
  check_moneypunct<wchar_t, true>(name, named);
  check_time<char>(name, named);
  check_time<wchar_t>(name, named);
  check_messages<char>(name, named);
  check_messages<wchar_t>(name, named);
  check_codecvt(name, named);
}

int main() {
  check_all("C");
  check_all("POSIX");
  for (const char* name : {"C.UTF-8", "C.utf8", "en_US.UTF-8"}) {
    try {
      std::locale probe(name);
    } catch (const std::runtime_error&) {
      continue;  // [locale.cons]: not a valid locale name here
    }
    check_all(name);
  }
  return 0;
}
