// [re.tokiter.general]: iterator_category is forward_iterator_tag and iterator_concept
// input_iterator_tag; /3 and /9: each match yields subs[0], subs[1], ... in turn, where -1 is the
// match's prefix(); /4, /7: with -1 among subs, a non-empty remainder after the last match is a
// final suffix token whose matched is true; /6: copies compare equal, end iterators are equal.
// [re.tokiter.cnstr]/3: the int, vector, initializer_list and array constructors store the same
// subs; /4: with no match at all and -1 among subs the iterator is a suffix iterator over [a, b),
// otherwise the end iterator. [re.tokiter.comp]: suffix iterators are equal iff their suffixes
// compare equal; an iterator differs from one at another sub-expression (N) of the same match.
// [re.tokiter.deref]: -> is the address of *. [re.tokiter.incr]/2-7, and the defaulted
// postfix ++, which returns the old value. A sub-expression index beyond the regex's yields
// the unmatched sub_match of [re.results.acc]/8.
#include <regex>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>
#include "check.hpp"

using It = std::cregex_token_iterator;
static_assert(std::is_same_v<It::iterator_category, std::forward_iterator_tag>);
static_assert(std::is_same_v<It::iterator_concept, std::input_iterator_tag>);
static_assert(std::is_same_v<It::value_type, std::csub_match>);
static_assert(std::is_same_v<It::reference, const std::csub_match&>);
static_assert(std::is_same_v<It::pointer, const std::csub_match*>);
static_assert(std::input_iterator<It>);

std::vector<std::string> collect(It it) {
  std::vector<std::string> out;
  for (; it != std::default_sentinel; ++it) out.push_back(it->str());
  return out;
}

int main() {
  const char s[] = "a1b22c";
  const char* e = s + sizeof s - 1;
  std::regex digits("(\\d)(\\d)?");

  // /9: -1 is the prefix of the current match, then the remaining suffix (/4).
  using V = std::vector<std::string>;
  CHECK((collect(It(s, e, digits, {-1, 0})) == V{"a", "1", "b", "22", "c"}));
  // Without -1 there is no suffix token.
  CHECK((collect(It(s, e, digits, {0})) == V{"1", "22"}));
  // An index the regex lacks (3 > mark_count()) and a group that did not participate: empty.
  It odd(s, e, digits, {2, 3});
  CHECK(!odd->matched && odd->str().empty());
  ++odd;
  CHECK(!odd->matched);
  ++odd;
  CHECK(odd->matched && odd->str() == "2");
  ++odd;
  CHECK(!odd->matched && ++odd == It());

  // [re.tokiter.cnstr]/3: the four ways of passing the sub-expressions.
  const int arr[] = {1, -1};
  std::vector<int> vec{1, -1};
  V expect{"1", "a", "2", "b", "c"};
  CHECK(collect(It(s, e, digits, arr)) == expect);
  CHECK(collect(It(s, e, digits, vec)) == expect);
  CHECK(collect(It(s, e, digits, {1, -1})) == expect);
  CHECK((collect(It(s, e, digits, 1)) == V{"1", "2"}));
  CHECK((collect(It(s, e, digits)) == V{"1", "22"})); // default 0

  // [re.tokiter.cnstr]/4: no match: a suffix iterator over the whole range if -1 is in subs.
  std::regex z("z");
  It whole(s, e, z, {0, -1});
  CHECK(whole != It() && whole->matched && whole->first == s && whole->second == e);
  CHECK(&*whole == whole.operator->());
  It after = whole++; // postfix: returns the suffix iterator, then becomes the end iterator
  CHECK(after->str() == "a1b22c" && whole == It() && whole == std::default_sentinel);
  CHECK(It(s, e, z, {0, 1}) == It());

  // [re.tokiter.incr]/5: an empty remainder gives no suffix token.
  const char t[] = "x1";
  CHECK((collect(It(t, t + 2, digits, -1)) == V{"x"}));

  // [re.tokiter.comp] / [re.tokiter.general]/6.
  It a(s, e, digits, {1, 2});
  It b(s, e, digits, {1, 2});
  CHECK(a == b);         // constructed from the same arguments
  It a2 = a;
  ++a2;                  // same match, N == 1
  CHECK(a2 != a && a2 != It());
  ++b;
  CHECK(a2 == b);
  CHECK(It() == It() && It() == std::default_sentinel);
  // Suffix iterators over different sequences compare equal iff their suffix strings do.
  const char u[] = "q9tail";
  const char w[] = "rr77tail";
  It su(u, u + 6, digits, -1), sw(w, w + 8, digits, -1);
  ++su; // "q" -> "tail"
  ++sw; // "rr" -> "tail"
  CHECK(su->str() == "tail" && sw->str() == "tail" && su->first != sw->first);
  CHECK(su == sw);
  It sx(s, e, digits, -1);
  ++sx;
  ++sx; // "a", "b", then the suffix "c"
  CHECK(sx->str() == "c" && sx != su && sx != It());

  // Copies are independent.
  It c1(s, e, digits, 0);
  It c2 = c1;
  ++c1;
  CHECK(c2->str() == "1" && c1->str() == "22");
  return 0;
}
