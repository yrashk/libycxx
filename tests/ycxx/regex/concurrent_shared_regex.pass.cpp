// [res.on.data.races]/2-3: the regex algorithms take the regular expression by const reference
// ([re.alg.match], [re.alg.search], [re.alg.replace]; regex_iterator holds a pointer to a
// const basic_regex, [re.regiter]), so several threads may match with the same basic_regex
// object at once, each with its own target and match_results; distinct basic_regex objects
// may be constructed and assigned concurrently ([re.regex.construct], [re.regex.assign]).
// The results are compared with those computed before the threads started. Meant to be run
// under TSan.
// FLAGS: -pthread
#include <cstddef>
#include <iterator>
#include <latch>
#include <regex>
#include <string>
#include <thread>
#include <vector>
#include "check.hpp"

constexpr int K = 4;

struct Shared {
  const std::regex date{R"((\d{4})-(\d\d)-(\d\d))"};
  const std::regex word{R"(\b[a-z]+\b)", std::regex::icase};
  const std::regex lines{"^x+$", std::regex::multiline};
  const std::regex basic{"a\\(b*\\)c", std::regex::basic};
  const std::regex alt{"(cat|dog)s?", std::regex::optimize};
  const std::wregex wide{L"[[:alpha:]]+[[:digit:]]"};
};

struct Out {
  bool m1 = false, m2 = false;
  std::string year, replaced, words, basic_group;
  std::size_t word_count = 0, line_count = 0, tokens = 0;
  std::wstring wmatch;
  bool operator==(const Out&) const = default;
};

static Out compute(const Shared& sh, int r) {
  Out o;
  const std::string text = "On 2026-10-0" + std::to_string(r % 10) + " the Cats and dogs met " +
                           std::to_string(r) + " times\nxxx\nyx\nxx";
  std::smatch m;
  o.m1 = std::regex_search(text, m, sh.date);
  if (o.m1) o.year = m[1].str() + "/" + m[3].str();
  o.m2 = std::regex_match(std::string("2000-01-02"), sh.date);
  o.replaced = std::regex_replace(text, sh.alt, "<$1>");
  for (std::sregex_iterator it(text.begin(), text.end(), sh.word), end; it != end; ++it) {
    ++o.word_count;
    o.words += it->str() + ",";
  }
  o.line_count = static_cast<std::size_t>(
      std::distance(std::sregex_iterator(text.begin(), text.end(), sh.lines), std::sregex_iterator()));
  std::cmatch cm;
  if (std::regex_search("xxabbbcyy", cm, sh.basic)) o.basic_group = cm[1].str();
  for (std::sregex_token_iterator it(text.begin(), text.end(), sh.date, {1, 2}), end; it != end; ++it) ++o.tokens;
  std::wsmatch wm;
  std::wstring wt = L"--abc" + std::to_wstring(r % 10) + L"--";
  if (std::regex_search(wt, wm, sh.wide)) o.wmatch = wm.str();
  return o;
}

int main() {
  const Shared sh;
  std::vector<Out> expected;
  for (int r = 0; r < 10; ++r) expected.push_back(compute(sh, r));
  CHECK(expected[3].m1 && expected[3].year == "2026/03" && expected[3].m2);
  CHECK(expected[3].replaced.find("the Cats and <dog> met") != std::string::npos);
  CHECK(expected[3].line_count == 2 && expected[3].basic_group == "bbb" && expected[3].tokens == 2);
  CHECK(expected[3].wmatch == L"abc3");
  std::latch go(K);
  std::vector<std::thread> ts;
  for (int k = 0; k < K; ++k)
    ts.emplace_back([&, k] {
      go.arrive_and_wait();
      for (int r = 0; r < 30; ++r) {
        const int i = (k * 3 + r) % 10;
        CHECK(compute(sh, i) == expected[static_cast<std::size_t>(i)]);
        // distinct regex objects, built and reassigned concurrently
        std::regex own("[0-9]+" + std::to_string(k));
        CHECK(std::regex_search("abc12" + std::to_string(k), own));
        own.assign("z{2,}", std::regex::extended);
        CHECK(std::regex_match("zzz", own) && !std::regex_match("z", own));
        std::regex copy = sh.alt;  // copy of the shared object
        CHECK(std::regex_match("dogs", copy) && copy.mark_count() == 1);
        CHECK(sh.date.mark_count() == 3 && sh.word.flags() & std::regex::icase);
      }
    });
  for (auto& t : ts) t.join();
}
