// [re.alg.search]/2-3: regex_search looks for a match at each position in turn. A pattern that
// begins with a single-character loop (".*foo", "\w+@", "[^,]*;", "a*?b") scans the run at every
// start position, and gives it back one character at a time; without remembering the runs it
// scanned that is quadratic in the input, and the step budget (DECISIONS §3, error_complexity:
// linear in the input reached) then fires on an ordinary 10 K line where every other
// implementation returns false. [re.err]/1 leaves the limits implementation-defined, but a
// search over 10^5 characters for such patterns must complete, and in time linear in the input:
// the characters read through the iterator are counted (a bidirectional iterator, as
// [re.alg.search]/1 requires no more), and must stay within a small multiple of the length.
// The results are compared with the known answers, found and not found.
// REQUIRES: exceptions
#include <regex>
#include <cstddef>
#include <iterator>
#include <string>
#include "check.hpp"

namespace rc = std::regex_constants;

// A bidirectional iterator over a std::string that counts the characters read.
static long reads = 0;
struct count_it {
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = char;
  using difference_type = std::ptrdiff_t;
  using pointer = const char*;
  using reference = const char&;
  const char* p = nullptr;
  const char& operator*() const {
    ++reads;
    return *p;
  }
  count_it& operator++() {
    ++p;
    return *this;
  }
  count_it operator++(int) {
    count_it t = *this;
    ++p;
    return t;
  }
  count_it& operator--() {
    --p;
    return *this;
  }
  count_it operator--(int) {
    count_it t = *this;
    --p;
    return t;
  }
  bool operator==(const count_it& o) const { return p == o.p; }
  bool operator!=(const count_it& o) const { return p != o.p; }
};

// Searches s with re; checks found and the match's position and length; returns the reads.
static long search(const std::string& s, const std::regex& re, bool found, long pos = 0, long len = 0,
                   rc::match_flag_type f = rc::match_default) {
  std::match_results<count_it> m;
  reads = 0;
  const bool ok = std::regex_search(count_it{s.data()}, count_it{s.data() + s.size()}, m, re, f);
  CHECK(ok == found);
  if (found) {
    CHECK(m.position() == pos);
    CHECK(m.length() == len);
  }
  return reads;
}

int main() {
  const long n = 100000;
  const std::string xs(n, 'x');
  // Every search reads each character a bounded number of times (the leading scan, the
  // give-back, and one more scan from where the run after the first failure continues).
  const long bound = 16 * n + 1000;
  CHECK(search(xs, std::regex(".*foo"), false) <= bound);
  CHECK(search(xs, std::regex("\\w+@"), false) <= bound);
  CHECK(search(xs, std::regex("[^,]*;"), false) <= bound);
  CHECK(search(xs, std::regex("a*b"), false) <= bound);
  CHECK(search(xs, std::regex("x*b"), false) <= bound);
  CHECK(search(xs, std::regex(".*?foo"), false) <= bound);   // lazy
  CHECK(search(xs, std::regex("x+?y"), false) <= bound);
  CHECK(search(xs, std::regex("(.*)foo"), false) <= bound);   // through a group
  CHECK(search(xs, std::regex("(?:x|y)*z"), false) <= bound); // not a single-character loop: the memo
  CHECK(search(xs, std::regex("x*foo", rc::multiline), false) <= bound);
  CHECK(search(xs, std::regex("x*foo", rc::icase), false) <= bound);
  CHECK(search(xs, std::regex("x*", rc::ECMAScript), true, 0, n) <= bound);
  // Found late: ".*" gives back to the last "foo"; "\w+@" to the "@".
  {
    std::string s = xs + "foo";
    CHECK(search(s, std::regex(".*foo"), true, 0, n + 3) <= bound);
    CHECK(search(s, std::regex("x*foo"), true, 0, n + 3) <= bound);
    s = xs + "@";
    CHECK(search(s, std::regex("\\w+@"), true, 0, n + 1) <= bound);
    CHECK(search(s, std::regex("\\w+?@"), true, 0, n + 1) <= bound);
    s = xs + ";";
    CHECK(search(s, std::regex("[^,]*;"), true, 0, n + 1) <= bound);
    s = "," + xs + ";";
    CHECK(search(s, std::regex("[^,]*;"), true, 1, n + 1) <= bound);
    s = xs + ",y;";
    CHECK(search(s, std::regex("[^,]*;"), true, n + 1, 2) <= bound);
    s = xs + "yfoo";
    CHECK(search(s, std::regex("x*foo"), true, n + 1, 3) <= bound);
  }
  // Many short runs: each is scanned once more from its first position only.
  {
    std::string s;
    for (long i = 0; i < n / 10; ++i) s += "aaaaaaaaa,";
    CHECK(search(s, std::regex("a*b"), false) <= bound);
    CHECK(search(s, std::regex("[^,]*;"), false) <= bound);
    CHECK(search(s, std::regex("a*,"), true, 0, 10) <= bound);
    s += "aab";
    CHECK(search(s, std::regex("a*b"), true, n, 3) <= bound);
  }
  // The remembered scan is bounded to the run it saw: after a loop fails at one position the
  // result at every other one is still right. Small cases against known answers.
  {
    std::smatch m;
    std::string s = "aaa,aab";
    CHECK(std::regex_search(s, m, std::regex("a*b")) && m.position() == 4 && m.length() == 3);
    CHECK(std::regex_search(s, m, std::regex("a*?b")) && m.position() == 4 && m.length() == 3);
    CHECK(std::regex_search(s, m, std::regex("a+b")) && m.position() == 4 && m.length() == 3);
    CHECK(std::regex_search(s, m, std::regex("a{2}b")) && m.position() == 4 && m.length() == 3);
    CHECK(!std::regex_search(s, m, std::regex("a{3}b")));
    CHECK(std::regex_search(s, m, std::regex("a{2,}b")) && m.position() == 4 && m.length() == 3);
    CHECK(std::regex_search(s, m, std::regex("[^,]*b")) && m.position() == 4 && m.length() == 3);
    CHECK(std::regex_search(s, m, std::regex(".*b")) && m.position() == 0 && m.length() == 7);
    CHECK(std::regex_search(s, m, std::regex("(a*)(b)")) && m.position() == 4 && m.str(1) == "aa");
    // a single-character loop inside a repeated group, and inside a lookahead
    s = "xaaaxaab";
    CHECK(std::regex_search(s, m, std::regex("(?:xa*)+b")) && m.position() == 0 && m.length() == 8);
    CHECK(std::regex_search(s, m, std::regex("(?:a*x)+a*b")) && m.position() == 0 && m.length() == 8);
    CHECK(std::regex_search(s, m, std::regex("(?=a*b)a+")) && m.position() == 5 && m.length() == 2);
    CHECK(std::regex_search(s, m, std::regex("(?!a*b)a+")) && m.position() == 1 && m.length() == 3);
    CHECK(std::regex_search(s, m, std::regex("(?:a*?x)+a*b")) && m.position() == 0 && m.length() == 8);
    // match_not_null with an empty loop: the empty match at 0 is refused, the next found
    s = "aab";
    CHECK(std::regex_search(s, m, std::regex("a*"), rc::match_not_null) && m.position() == 0 && m.length() == 2);
    CHECK(std::regex_search(s, m, std::regex("b*"), rc::match_not_null) && m.position() == 2 && m.length() == 1);
  }
  return 0;
}
