// Exception-injection sweep over basic_regex construction and assignment, the regex algorithms
// with a match_results whose allocator throws, regex_iterator/regex_token_iterator and
// regex_replace: operator new (and exh::alloc's allocate for match_results) fails at its k-th
// call, for every k until the operation completes.
//   [re.regex.assign]/10: assign(s, f): "If an exception is thrown, *this is unchanged."
//     (operator=(const charT*), operator=(const basic_string&), assign(const charT*[, len]),
//     assign(first, last) and assign(il) are specified as calls of it, /3-/9, /13-14.)
//   [res.on.exception.handling]/1, [re.results]: no further guarantee for the algorithms and
//     iterators: every operator new block and every allocator block is freed exactly once.
#include <regex>
#include <string>
#include "exc_new.hpp"

using namespace exh;
using SM = std::sub_match<const char*>;
using MR = std::match_results<const char*, alloc<SM>>;

static const char text[] = "alpha=1, beta=22, gamma=333, delta=4444";

static bool behaves_like_old(const std::regex& r) {
  // the old pattern is "(\\w+)=(\\d+)": two marked sub-expressions, matches "beta=22"
  return r.mark_count() == 2 && r.flags() == std::regex::ECMAScript && std::regex_match("beta=22", r) &&
         !std::regex_match("beta=", r);
}

template <class F>
void sw(const char* name, std::initializer_list<Kind> ks, F f) {
  for (Kind k : ks) {
    if (k == gnew)
      sweep_new(name, f);
    else
      sweep(name, k, new_balanced(f));
  }
}

int main() {
  sw("regex(pattern)", {gnew}, [] { return attempt([] { std::regex r("(a|b)*c[0-9]{2,5}(x+?)\\1"); }); });
  sw("regex(pattern, icase|multiline)", {gnew}, [] {
    return attempt([] { std::regex r("^(\\w+)\\s*$", std::regex::icase | std::regex::multiline); });
  });
  for (int which = 0; which < 5; ++which)
    sw("regex::assign: *this unchanged on an exception", {gnew}, [which] {
      std::regex r("(\\w+)=(\\d+)");
      static const std::string pat = "x(y|z)+[a-f]{3}";
      bool threw = attempt([&] {
        switch (which) {
        case 0: r.assign("x(y|z)+[a-f]{3}"); break;
        case 1: r = "x(y|z)+[a-f]{3}"; break;
        case 2: r = pat; break;
        case 3: r.assign(pat.begin(), pat.end(), std::regex::extended); break;
        case 4: r.assign({'q', '(', 'r', ')', '*'}); break;
        }
      });
      if (threw) EXH_EXPECT(behaves_like_old(r), "[re.regex.assign]/10: *this changed although assign threw");
      return threw;
    });
  // An invalid pattern: regex_error, and *this unchanged (no injection needed: one run).
  {
    std::regex r("(\\w+)=(\\d+)");
    bool caught = false;
    try {
      r.assign("(unclosed");
    } catch (const std::regex_error&) {
      caught = true;
    }
    if (!caught || !behaves_like_old(r)) report("[re.regex.assign]/10: regex_error left *this changed", __LINE__);
  }
  sw("regex copy construction/assignment", {gnew}, [] {
    std::regex a("(\\w+)=(\\d+)"), b("q+");
    bool threw = attempt([&] {
      std::regex c(a);
      b = a;
    });
    return threw;
  });

  static const std::regex re("(\\w+)=(\\d+)");
  sw("regex_search with match_results<alloc>", {allocation, gnew}, [] {
    MR m;
    return attempt([&] { std::regex_search(text, m, re); });
  });
  sw("regex_match with match_results<alloc>", {allocation, gnew}, [] {
    MR m;
    return attempt([&] { std::regex_match("gamma=333", m, re); });
  });
  sw("match_results<alloc> copy and format", {allocation, gnew}, [] {
    MR m;
    std::regex_search(text, m, re);
    return attempt([&] {
      MR copy(m);
      std::string f = m.format("$2:$1");
    });
  });
  sw("regex_iterator traversal", {gnew}, [] {
    return attempt([] {
      int n = 0;
      for (std::cregex_iterator it(text, text + sizeof text - 1, re), end; it != end; ++it) n += int(it->size());
    });
  });
  sw("regex_token_iterator traversal", {gnew}, [] {
    return attempt([] {
      int subs[] = {1, 2, -1};
      std::size_t total = 0;
      for (std::cregex_token_iterator it(text, text + sizeof text - 1, re, subs), end; it != end; ++it)
        total += it->str().size();
    });
  });
  sw("regex_replace", {gnew}, [] {
    return attempt([] { std::string s = std::regex_replace(std::string(text), re, "[$2/$1]"); });
  });
  return finish();
}
