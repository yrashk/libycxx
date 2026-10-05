// [re.submatch]: sub_match<BiIter> is a pair of iterators with matched; length() is the
// distance (0 if !matched), str() and the conversion to string_type the matched text (empty
// if !matched), compare() compares str(); == and <=> are provided against sub_match,
// string_type, const value_type* and value_type. [re.results]: smatch/cmatch/wsmatch/wcmatch
// are match_results specializations.
// COUNTERPART: libcxx:re/re.submatch/re.submatch.op/compare.pass.cpp
#include <regex>
#include <compare>
#include <string>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<std::csub_match, std::sub_match<const char*>>);
static_assert(std::is_same_v<std::ssub_match, std::sub_match<std::string::const_iterator>>);
static_assert(std::is_same_v<std::cmatch, std::match_results<const char*>>);
static_assert(std::is_same_v<std::smatch::value_type, std::ssub_match>);
static_assert(std::is_base_of_v<std::pair<const char*, const char*>, std::csub_match>);
static_assert(std::is_same_v<std::csub_match::string_type, std::string>);

int main() {
  std::cmatch m;
  CHECK(std::regex_search("say hello world", m, std::regex("(h\\w+) (w\\w+)|(z)")));
  const std::csub_match& h = m[1];
  CHECK(h.matched && h.length() == 5 && h.str() == "hello" && std::string(h) == "hello");
  CHECK(h == "hello" && "hello" == h && h == std::string("hello") && h != "world");
  CHECK(h < m[2] && (h <=> m[2]) < 0 && h.compare(m[2]) < 0 && h.compare("hello") == 0);
  CHECK(h.compare(std::string("hellp")) < 0);
  CHECK(m[3].length() == 0 && !m[3].matched && m[3].str().empty() && m[3] == "");
  std::csub_match one;
  one.first = "x";
  one.second = one.first + 1;
  one.matched = true;
  CHECK(one == 'x' && 'x' == one && one < 'y');
  std::csub_match def;
  CHECK(!def.matched && def.length() == 0);
  return 0;
}
