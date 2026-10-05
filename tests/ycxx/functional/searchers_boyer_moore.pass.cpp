// [func.search.bm], [func.search.bmh]: boyer_moore_searcher and boyer_moore_horspool_searcher.
//   Constructor: pattern [pat_first, pat_last), Hash hf = Hash() (default hash<value_type>),
//   BinaryPredicate pred = BinaryPredicate() (default equal_to<>); Preconditions: pred(A, B)
//   implies hf(A) == hf(B).
//   operator()(first, last): Returns "A pair of iterators i and j such that i is the first
//   iterator in the range [first, last - (pat_last_ - pat_first_)) such that for every
//   non-negative integer n less than pat_last_ - pat_first_ ... pred(*(i + n), *(pat_first_ +
//   n)) != false, and j == next(i, distance(pat_first_, pat_last_)). Returns make_pair(first,
//   first) if [pat_first_, pat_last_) is empty, otherwise returns make_pair(last, last) if no
//   such iterator is found." Complexity: at most (last - first) * (pat_last_ - pat_first_)
//   applications of the predicate.
//   [alg.search]: search(first, last, searcher) returns searcher(first, last).first.
// The searchers are const-callable and reusable; class template argument deduction works from
// the constructor. (The quoted range excludes a match that ends exactly at last; every check
// below either has such a match spelled out separately or keeps matches away from the end, so
// that the test does not depend on that reading.)
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>
#include <random>
#include <string>
#include <utility>
#include <vector>
#include "check.hpp"

template <template <class...> class S>
void strings() {
  const std::string text = "here is a simple example, a simple exam!";
  const std::string pat = "example";
  const S s(pat.begin(), pat.end());
  auto [i, j] = s(text.begin(), text.end());
  CHECK(i - text.begin() == 17 && j - i == 7);
  CHECK(std::search(text.begin(), text.end(), s) == text.begin() + 17);
  // Reuse on another text, through a const object.
  const std::string other = "no match here?";
  auto r = s(other.begin(), other.end());
  CHECK(r.first == other.end() && r.second == other.end());
  // Empty pattern: (first, first).
  const std::string empty;
  const S e(empty.begin(), empty.end());
  auto r2 = e(text.begin() + 3, text.end());
  CHECK(r2.first == text.begin() + 3 && r2.second == text.begin() + 3);
  // Pattern longer than the text.
  const std::string tiny = "exam";
  auto r3 = s(tiny.begin(), tiny.end());
  CHECK(r3.first == tiny.end() && r3.second == tiny.end());
  // First of several occurrences, overlapping ones included.
  const std::string aaa = "xaaaaay";
  const std::string aa = "aa";
  const S sa(aa.begin(), aa.end());
  CHECK(sa(aaa.begin(), aaa.end()).first == aaa.begin() + 1);
  const std::string abab = "abacababcab!";
  const std::string pab = "abab";
  const S sp(pab.begin(), pab.end());
  CHECK(sp(abab.begin(), abab.end()).first == abab.begin() + 4);
}

struct CaseHash {
  std::size_t operator()(char c) const { return std::hash<int>{}(std::tolower(static_cast<unsigned char>(c))); }
};
struct CaseEq {
  bool operator()(char a, char b) const {
    return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
  }
};

template <template <class...> class S>
void custom_predicate() {
  const std::string text = "The Quick Brown Fox jumps.";
  const std::string pat = "bROWN f";
  const S<std::string::const_iterator, CaseHash, CaseEq> s(pat.begin(), pat.end(), CaseHash{}, CaseEq{});
  auto [i, j] = s(text.begin(), text.end());
  CHECK(i - text.begin() == 10 && j - i == 7);
}

template <template <class...> class S>
void ints_and_oracle() {
  // Large and negative values: the default hash is used for a non-char value type.
  const std::vector<long> text = {5, -1000000007, 3, 1L << 40, 3, 1L << 40, 3, 9, 0};
  const std::vector<long> pat = {3, 1L << 40, 3, 9};
  const S s(pat.begin(), pat.end());
  CHECK(s(text.begin(), text.end()).first == text.begin() + 4);

  // Random texts over a small alphabet against default_searcher, counting predicate calls.
  std::mt19937 gen(12345);
  for (int round = 0; round < 3000; ++round) {
    std::string t, p;
    const int tn = static_cast<int>(gen() % 60), pn = 1 + static_cast<int>(gen() % 6);
    for (int k = 0; k < tn; ++k) t += static_cast<char>('a' + gen() % 3);
    for (int k = 0; k < pn; ++k) p += static_cast<char>('a' + gen() % 3);
    t += '#';  // no match ends at last
    long calls = 0;
    auto eq = [&calls](char a, char b) {
      ++calls;
      return a == b;
    };
    const S<std::string::const_iterator, std::hash<char>, decltype(eq)> s2(p.cbegin(), p.cend(), std::hash<char>{}, eq);
    calls = 0;  // the bound is on operator(), not on the constructor's preprocessing
    const auto got = s2(t.cbegin(), t.cend());
    const auto want = std::default_searcher(p.cbegin(), p.cend())(t.cbegin(), t.cend());
    const long used = calls;
    CHECK(got == want);
    CHECK(used <= static_cast<long>(t.size()) * static_cast<long>(p.size()));
  }
}

int main() {
  strings<std::boyer_moore_searcher>();
  strings<std::boyer_moore_horspool_searcher>();
  custom_predicate<std::boyer_moore_searcher>();
  custom_predicate<std::boyer_moore_horspool_searcher>();
  ints_and_oracle<std::boyer_moore_searcher>();
  ints_and_oracle<std::boyer_moore_horspool_searcher>();
  return 0;
}
