// [re.submatch.members]/1: the default constructor value-initializes first, second and matched.
// /2-4: length(), str() and the conversion to string_type are 0 / empty when !matched, whatever
// first and second hold. /5-7: compare() is str().compare(...). /8-10: swap exchanges the pair
// and matched, noexcept iff the iterator's swap is.
// [re.submatch.op]/1-9: the comparisons go through compare() with a string_type built from the
// other operand, and <=> returns SM-CAT(BiIter), the comparison category of
// basic_string<value_type> (with its default traits, whatever the other string's traits are);
// /8-9: a single character compares as string_type(1, c). /10: << inserts str().
#include <regex>
#include <compare>
#include <cstring>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include "check.hpp"

// Case-insensitive traits: a basic_string with them compares "HELLO" equal to "hello" itself,
// but [re.submatch.op]/4 converts it to sub_match's string_type (std::string) first.
struct ci_traits : std::char_traits<char> {
  static int lower(char c) { return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c; }
  static bool eq(char a, char b) { return lower(a) == lower(b); }
  static bool lt(char a, char b) { return lower(a) < lower(b); }
  static int compare(const char* a, const char* b, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i)
      if (lower(a[i]) != lower(b[i])) return lower(a[i]) < lower(b[i]) ? -1 : 1;
    return 0;
  }
};
using ci_string = std::basic_string<char, ci_traits>;

static_assert(std::is_same_v<decltype(std::declval<std::csub_match>() <=> std::declval<std::csub_match>()),
                             std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::declval<std::csub_match>() <=> "x"), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::declval<std::csub_match>() <=> 'x'), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::declval<std::csub_match>() <=> std::declval<ci_string>()),
                             std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::declval<std::wcsub_match>() <=> L"x"), std::strong_ordering>);
static_assert(noexcept(std::declval<std::csub_match&>().swap(std::declval<std::csub_match&>())));
static_assert(std::is_same_v<decltype(std::declval<const std::csub_match&>().length()),
                             std::csub_match::difference_type>);

std::csub_match make(const char* b, const char* e, bool matched) {
  std::csub_match m;
  m.first = b;
  m.second = e;
  m.matched = matched;
  return m;
}

int main() {
  // /1: value-initialized.
  std::csub_match def;
  CHECK(def.first == nullptr && def.second == nullptr && !def.matched);

  const char* text = "HELLO hello ab";
  // /2-4: an unmatched sub_match spanning characters still has length 0 and an empty string.
  std::csub_match ghost = make(text, text + 5, false);
  CHECK(ghost.length() == 0 && ghost.str().empty() && std::string(ghost).empty());
  CHECK(ghost == "" && ghost.compare("") == 0 && ghost.compare(std::string()) == 0);

  std::csub_match upper = make(text, text + 5, true);   // "HELLO"
  std::csub_match lower = make(text + 6, text + 11, true); // "hello"
  std::csub_match ab = make(text + 12, text + 14, true);   // "ab"
  CHECK(upper.length() == 5 && upper.str() == "HELLO");

  // /5-7: compare() is std::string::compare on str().
  CHECK(upper.compare(lower) < 0 && lower.compare(upper) > 0 && upper.compare(upper) == 0);
  CHECK(upper.compare(std::string("HELLO")) == 0 && upper.compare("HELLP") < 0);

  // [re.submatch.op]/2-3.
  CHECK(upper != lower && (upper <=> lower) == std::strong_ordering::less);
  CHECK((lower <=> upper) == std::strong_ordering::greater && upper == make(text, text + 5, true));
  // /4-5: the other string's traits are not used: "hello" in ci_string is not "HELLO".
  ci_string ci_hello("hello");
  CHECK(ci_string("HELLO") == ci_hello); // the traits themselves are case-insensitive
  CHECK(upper != ci_hello && lower == ci_hello && ci_hello == lower);
  CHECK((upper <=> ci_hello) == std::strong_ordering::less && (ci_hello <=> upper) == std::strong_ordering::greater);
  // /6-7: a null-terminated string.
  CHECK(lower == "hello" && "hello" == lower && (lower <=> "hellp") < 0 && ("hellp" <=> lower) > 0);
  // /8-9: a character is a one-character string: "ab" > "a", "ab" < "b", "ab" != 'a'.
  CHECK(ab != 'a' && (ab <=> 'a') > 0 && (ab <=> 'b') < 0 && ('b' <=> ab) > 0);
  CHECK(make(text + 12, text + 13, true) == 'a');
  CHECK(ghost != 'a' && (ghost <=> 'a') < 0); // "" < "a"

  // /10: inserts str(), so nothing for an unmatched one.
  std::ostringstream os;
  os << '[' << upper << '|' << ghost << '|' << ab << ']';
  CHECK(os.str() == "[HELLO||ab]");
  std::wostringstream wos;
  const wchar_t* wtext = L"wide";
  std::wcsub_match w;
  w.first = wtext;
  w.second = wtext + 4;
  w.matched = true;
  wos << w;
  CHECK(wos.str() == L"wide" && w == L"wide");

  // [re.submatch.members]/8-10: swap exchanges first, second and matched.
  std::csub_match x = upper, y = ghost;
  x.swap(y);
  CHECK(x.first == text && x.second == text + 5 && !x.matched);
  CHECK(y.first == text && y.second == text + 5 && y.matched && y.str() == "HELLO");
  std::csub_match z = ab;
  y.swap(z);
  CHECK(y.str() == "ab" && z.str() == "HELLO");
  return 0;
}
