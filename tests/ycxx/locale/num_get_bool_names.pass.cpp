// [facet.num.get.virtuals]/7-8 (boolalpha): "Successive characters in the range [in, end) are
// obtained and matched against corresponding positions in the target sequences only as
// necessary to identify a unique match. The input iterator in is compared to end only when
// necessary to obtain a character. If a target sequence is uniquely matched, val is set to the
// corresponding value. Otherwise false is stored and ios_base::failbit is assigned to err."
// "The in iterator is always left pointing one position beyond the last character successfully
// matched. If val is set, then err is set to str.goodbit; or to str.eofbit if, when seeking
// another character to match, it is found that (in == end). If val is not set, then err is set
// to str.failbit; or to (str.failbit | str.eofbit) if the reason for the failure was that
// (in == end)." Example 2: targets true "a", false "abb": "a" yields val == true and err ==
// str.eofbit; "abc" yields err = str.failbit, with in ending at the 'c' element. Targets "1" /
// "0": "1" yields true and goodbit. "For empty targets (""), any input sequence yields err ==
// str.failbit."
#include <locale>
#include <sstream>
#include <iterator>
#include <string>
#include "check.hpp"

struct Names : std::numpunct<char> {
  std::string t, f;
  Names(std::string tn, std::string fn) : t(tn), f(fn) {}
  std::string do_truename() const override { return t; }
  std::string do_falsename() const override { return f; }
};

static bool get(const std::locale& loc, const char* s, std::ios_base::iostate& err, std::string& rest,
                bool init) {
  std::istringstream is(s);
  is.imbue(loc);
  is.setf(std::ios_base::boolalpha);
  using It = std::istreambuf_iterator<char>;
  bool v = init;
  err = std::ios_base::goodbit;
  It it = std::use_facet<std::num_get<char>>(loc).get(It(is), It(), is, err, v);
  rest.clear();
  for (; it != It(); ++it) rest.push_back(*it);
  return v;
}

int main() {
  using B = std::ios_base;
  B::iostate err;
  std::string rest;
  const std::locale c = std::locale::classic();

  // Example 2.
  const std::locale ex(c, new Names("a", "abb"));
  CHECK(get(ex, "a", err, rest, false) == true && err == B::eofbit && rest.empty());
  CHECK(get(ex, "abc", err, rest, true) == false && err == B::failbit && rest == "c");
  CHECK(get(ex, "abb", err, rest, true) == false && err == B::goodbit && rest.empty());
  CHECK(get(ex, "abbz", err, rest, true) == false && err == B::goodbit && rest == "z");
  CHECK(get(ex, "ab", err, rest, true) == false && err == (B::failbit | B::eofbit));
  CHECK(get(ex, "x", err, rest, true) == false && err == B::failbit && rest == "x");
  CHECK(get(ex, "", err, rest, true) == false && err == (B::failbit | B::eofbit));

  const std::locale digits(c, new Names("1", "0"));
  CHECK(get(digits, "1", err, rest, false) == true && err == B::goodbit);
  CHECK(get(digits, "0x", err, rest, true) == false && err == B::goodbit && rest == "x");

  const std::locale empty(c, new Names("", ""));
  CHECK(get(empty, "true", err, rest, true) == false && (err & B::failbit));
  CHECK(get(empty, "", err, rest, true) == false && (err & B::failbit));

  // Default names: a unique match is identified at the first character.
  CHECK(get(c, "truex", err, rest, false) == true && err == B::goodbit && rest == "x");
  CHECK(get(c, "falsey", err, rest, true) == false && err == B::goodbit && rest == "y");
  CHECK(get(c, "fals", err, rest, true) == false && err == (B::failbit | B::eofbit));
  CHECK(get(c, "trUe", err, rest, false) == false && err == B::failbit && rest == "Ue");
  CHECK(get(c, " true", err, rest, true) == false && err == B::failbit && rest == " true");

  // Identical names: never a unique match.
  const std::locale same(c, new Names("yes", "yes"));
  CHECK(get(same, "yes", err, rest, true) == false && (err & B::failbit));

  // A shared prefix that is decided late.
  const std::locale pre(c, new Names("ok", "okay"));
  CHECK(get(pre, "ok", err, rest, false) == true && err == B::eofbit);
  CHECK(get(pre, "okay!", err, rest, true) == false && err == B::goodbit && rest == "!");
  CHECK(get(pre, "oka", err, rest, true) == false && err == (B::failbit | B::eofbit));
  // "x" is not matched, so in stays at it and "ok" is the unique match (unlike Example 2's
  // "abc", where the matched 'b' has moved in past the end of the true name "a").
  CHECK(get(pre, "okx", err, rest, false) == true && err == B::goodbit && rest == "x");

  // Without boolalpha: "as it would for a long"; 0 -> false, 1 -> true, otherwise true and
  // failbit (also for a long out of range).
  auto num = [](const char* s, B::iostate& e) {
    std::istringstream is(s);
    using It = std::istreambuf_iterator<char>;
    bool v = false;
    e = B::goodbit;
    std::use_facet<std::num_get<char>>(is.getloc()).get(It(is), It(), is, e, v);
    return v;
  };
  CHECK(num("-1", err) == true && (err & B::failbit));
  CHECK(num("00", err) == false && err == B::eofbit);
  CHECK(num("0x1", err) == false && err == B::goodbit);  // %d: "0" accumulated
  CHECK(num("99999999999999999999999", err) == true && (err & B::failbit));
  return 0;
}
