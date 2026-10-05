// The collation customization points of a user regex traits class ([re.req] Table 120).
// [re.grammar]/14.2: "comparison of a collating element range c1-c2 against a character c is
// conducted as follows: if flags() & regex_constants::collate is false then the character c is
// matched if c1 <= c && c <= c2, otherwise" c is matched iff
// transform(str1) <= transform(str) <= transform(str2), each a one-character string of
// translate (or translate_nocase with icase) of c1, c, c2.
// /14.3: membership in a primary equivalence class [[=name=]] compares the sort keys of
// transform_primary for equality (with or without collate).
// /4, /8, /10: [.name.] and [=name=] in a bracket expression; the name is "not valid if the value
// returned by traits_inst.lookup_collatename for that name is an empty string" (and for [=name=]
// also if transform_primary of it is empty); /11: an invalid name makes the constructor throw
// regex_error ([re.err]: error_collate for an invalid collating element name).
// Here transform sorts 'x' between 'a' and 'b'; transform_primary makes 'e', 'E' and '3'
// equivalent; lookup_collatename knows the multi-character name "dash" for '-'.
#include <locale>
#include <regex>
#include <string>
#include "check.hpp"

int transform_calls = 0, primary_calls = 0, collatename_calls = 0;

struct Traits {
  using char_type = char;
  using string_type = std::string;
  using locale_type = std::locale;
  using char_class_type = unsigned;
  enum : unsigned { digit = 1, alpha = 2, space = 4, under = 8 };
  std::locale loc;

  static std::size_t length(const char* p) { return std::char_traits<char>::length(p); }
  char translate(char c) const { return c; }
  char translate_nocase(char c) const {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
  }
  template <class It>
  std::string transform(It f, It l) const {
    ++transform_calls;
    std::string r;
    for (; f != l; ++f) {
      if (*f == 'x')
        r += "a~";  // a < x < b
      else
        r += *f;
    }
    return r;
  }
  template <class It>
  std::string transform_primary(It f, It l) const {
    ++primary_calls;
    std::string r;
    for (; f != l; ++f) {
      char c = translate_nocase(*f);
      r += c == '3' ? 'e' : c;
    }
    return r;
  }
  template <class It>
  std::string lookup_collatename(It f, It l) const {
    ++collatename_calls;
    std::string s(f, l);
    if (s == "dash") return "-";
    return s.size() == 1 ? s : std::string();
  }
  template <class It>
  unsigned lookup_classname(It f, It l, bool = false) const {
    std::string s(f, l);
    if (s == "d" || s == "digit") return digit;
    if (s == "alpha") return alpha;
    if (s == "s" || s == "space") return space;
    if (s == "w") return alpha | digit | under;
    return 0;
  }
  bool isctype(char c, unsigned cl) const {
    return ((cl & digit) && c >= '0' && c <= '9') ||
           ((cl & alpha) && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) ||
           ((cl & space) && (c == ' ' || c == '\t' || c == '\n')) || ((cl & under) && c == '_');
  }
  int value(char c, int radix) const {
    int v = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
    return v < radix ? v : -1;
  }
  locale_type imbue(locale_type l) {
    std::swap(loc, l);
    return l;
  }
  locale_type getloc() const { return loc; }
};

using R = std::basic_regex<char, Traits>;
namespace rc = std::regex_constants;

static bool full(const std::string& s, const R& re) { return std::regex_match(s.begin(), s.end(), re); }

int main() {
  // Ranges: plain comparison without collate, transform with collate.
  const R plain("[a-b]");
  CHECK(full("a", plain) && full("b", plain) && !full("x", plain) && !full("c", plain));
  transform_calls = 0;
  const R coll("[a-b]", rc::ECMAScript | rc::collate);
  CHECK(full("a", coll) && transform_calls > 0 && full("b", coll) && full("x", coll) && !full("c", coll) && !full("A", coll));
  // With icase and collate, the characters go through translate_nocase first.
  const R coll_icase("[a-b]", rc::ECMAScript | rc::collate | rc::icase);
  CHECK(full("A", coll_icase) && full("X", coll_icase) && !full("C", coll_icase));
  const R neg("[^a-b]", rc::ECMAScript | rc::collate);
  CHECK(!full("x", neg) && full("c", neg));

  // Equivalence classes use transform_primary, with or without collate.
  primary_calls = 0;
  const R eq("[[=e=]]+");
  CHECK(primary_calls > 0);
  CHECK(full("eE3e", eq) && !full("f", eq) && !full("e4", eq));
  CHECK(full("33", R("[[=E=]]+", rc::ECMAScript | rc::collate)));
  CHECK(full("q3", R("[q[=e=]]+")));
  CHECK(!full("e", R("[^[=3=]]")) && full("f", R("[^[=3=]]")));

  // Collating elements use lookup_collatename.
  collatename_calls = 0;
  const R dash("[[.dash.]]");
  CHECK(collatename_calls > 0);
  CHECK(full("-", dash) && !full("d", dash));
  CHECK(full("a-b", R("a[[.dash.][.x.]]b")) && full("axb", R("a[[.dash.][.x.]]b")));
  CHECK(full("x", R("[[.x.]]")));

  // Invalid names: lookup_collatename returns "".
  auto throws_collate = [](const char* p) {
    try {
      R r(p);
    } catch (const std::regex_error& e) {
      return e.code() == rc::error_collate;
    }
    return false;
  };
  CHECK(throws_collate("[[.bogus.]]"));
  CHECK(throws_collate("[[=bogus=]]"));
  CHECK(throws_collate("a[[.dash.][.nope.]]"));
  return 0;
}
