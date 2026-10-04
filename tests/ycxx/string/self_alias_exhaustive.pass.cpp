// Exhaustive check of the basic_string modifiers whose source characters lie inside *this,
// for every position, length and source window of short and long strings, with and without
// spare capacity (so both the in-place and the reallocating paths are taken).
// [string.replace]/12-13 replace(pos1, n1, s, n2): "Equivalent to: return replace(pos1, n1,
// basic_string_view<charT, traits>(s, n2));" with /4 (string_view form): "Replaces the
// characters in the range [begin() + pos1, begin() + pos1 + xlen) with the characters in the
// range [s, s + n2)" -- the values the source has before the call. [string.insert]/6-7
// insert(pos, s, n): "Inserts a copy of the range [s, s + n) immediately before the character
// at position pos". [string.append]/7: append(s, n) "Appends a copy of the range [s, s + n)".
// [string.assign]/9: assign(s, n) "Replaces the string controlled by *this with a copy of the
// range [s, s + n)". [string.copy] / replace(pos1, n1, n2, c) are also covered for every pos.
// Nothing in the draft restricts s to point outside *this.
#include <string>
#include <cstddef>
#include "check.hpp"

static std::string model_replace(const std::string& base, std::size_t pos, std::size_t n1, const std::string& src) {
  std::size_t xlen = n1 < base.size() - pos ? n1 : base.size() - pos;
  std::string r;
  for (std::size_t i = 0; i < pos; ++i) r.push_back(base[i]);
  for (char c : src) r.push_back(c);
  for (std::size_t i = pos + xlen; i < base.size(); ++i) r.push_back(base[i]);
  return r;
}

static void run(const std::string& init, bool spare) {
  const std::size_t sz = init.size();
  auto fresh = [&] {
    std::string s(init);
    if (spare) s.reserve(2 * sz + 64);
    else s.shrink_to_fit();
    return s;
  };
  for (std::size_t pos = 0; pos <= sz; ++pos)
    for (std::size_t n1 = 0; n1 <= sz - pos + 1; ++n1)
      for (std::size_t sp = 0; sp <= sz; ++sp)
        for (std::size_t n2 = 0; sp + n2 <= sz; ++n2) {
          const std::string src = init.substr(sp, n2);
          {
            std::string s = fresh();
            s.replace(pos, n1, s.data() + sp, n2);
            CHECK(s == model_replace(init, pos, n1, src));
          }
          if (n1 == 0) {
            std::string s = fresh();
            s.insert(pos, s.data() + sp, n2);
            CHECK(s == model_replace(init, pos, 0, src));
          }
          if (pos == 0 && n1 == 0) {
            std::string a = fresh();
            a.append(a.data() + sp, n2);
            CHECK(a == init + src);
            std::string b = fresh();
            b.assign(b.data() + sp, n2);
            CHECK(b == src);
            std::string c = fresh();
            c.replace(c.begin() + static_cast<std::ptrdiff_t>(sp), c.begin() + static_cast<std::ptrdiff_t>(sp + n2),
                      c.begin(), c.end());
            CHECK(c == model_replace(init, sp, n2, init));
          }
        }
  for (std::size_t pos = 0; pos <= sz; ++pos)
    for (std::size_t n1 = 0; n1 <= sz - pos + 1; ++n1)
      for (std::size_t n2 : {0u, 1u, 3u, 40u}) {
        std::string s = fresh();
        s.replace(pos, n1, n2, s.empty() ? 'z' : s[0]);
        CHECK(s == model_replace(init, pos, n1, std::string(n2, init.empty() ? 'z' : init[0])));
      }
}

int main() {
  for (const char* init : {"", "a", "abcdef", "0123456789abcdefghijklmnopqrstuvw"}) {
    run(init, false);
    run(init, true);
  }
  return 0;
}
