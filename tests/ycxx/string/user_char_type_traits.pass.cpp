// basic_string and basic_string_view over a program-defined char-like type ([strings.general]:
// trivially copyable, standard-layout, trivially default-constructible) whose traits treat two
// characters as equal when their first byte agrees, whatever the second byte (a tag) holds.
// Every character operation goes through the traits ([char.traits.general]/1; Table 87):
// - [string.find], [string.view.find]: find, rfind, find_first_of, find_last_of,
//   find_first_not_of, find_last_not_of are defined by traits::eq on the characters (the string
//   forms "as if by" the string_view forms); checked against a direct transcription of the
//   definitions for every needle/haystack/pos of a small alphabet;
// - [string.view.ops] compare: traits::compare; [string.view.comparison] ==, <=> (R is
//   weak_ordering when the traits have no comparison_category), starts_with, ends_with,
//   contains;
// - [string.cons] basic_string(const charT* s): length traits::length(s), "the smallest i such
//   that X::eq(p[i], charT())" -- a character whose first byte is 0 ends the string even when
//   its tag is not;
// - [string.copy] copy(s, n, pos): "Equivalent to traits::copy(s, data() + pos, rlen)",
//   [string.view.ops] copy likewise; the traits count their copy calls.
// A library comparing the bytes of the characters (memcmp, memchr, ==) gets these wrong.
#include <algorithm>
#include <compare>
#include <cstddef>
#include <cwchar>
#include <iosfwd>
#include <string>
#include <string_view>
#include <type_traits>
#include "check.hpp"

struct UC {
  unsigned char c;
  unsigned char tag;
};
static_assert(std::is_trivially_copyable_v<UC> && std::is_standard_layout_v<UC> &&
              std::is_trivially_default_constructible_v<UC>);

static int copy_calls = 0;

struct UTraits {
  using char_type = UC;
  using int_type = int;
  using off_type = std::streamoff;
  using pos_type = std::streampos;
  using state_type = std::mbstate_t;
  static constexpr void assign(UC& r, const UC& d) noexcept { r = d; }
  static constexpr bool eq(UC a, UC b) noexcept { return a.c == b.c; }
  static constexpr bool lt(UC a, UC b) noexcept { return a.c < b.c; }
  static constexpr int compare(const UC* p, const UC* q, std::size_t n) noexcept {
    for (std::size_t i = 0; i < n; ++i) {
      if (lt(p[i], q[i])) return -1;
      if (lt(q[i], p[i])) return 1;
    }
    return 0;
  }
  static constexpr std::size_t length(const UC* p) noexcept {
    std::size_t i = 0;
    while (!eq(p[i], UC())) ++i;
    return i;
  }
  static constexpr const UC* find(const UC* p, std::size_t n, const UC& c) noexcept {
    for (std::size_t i = 0; i < n; ++i)
      if (eq(p[i], c)) return p + i;
    return nullptr;
  }
  static constexpr UC* move(UC* s, const UC* p, std::size_t n) noexcept {
    if (s < p)
      for (std::size_t i = 0; i < n; ++i) assign(s[i], p[i]);
    else
      for (std::size_t i = n; i > 0; --i) assign(s[i - 1], p[i - 1]);
    return s;
  }
  static UC* copy(UC* s, const UC* p, std::size_t n) noexcept {
    ++copy_calls;
    for (std::size_t i = 0; i < n; ++i) assign(s[i], p[i]);
    return s;
  }
  static constexpr UC* assign(UC* s, std::size_t n, UC c) noexcept {
    for (std::size_t i = 0; i < n; ++i) assign(s[i], c);
    return s;
  }
  static constexpr int_type not_eof(int_type e) noexcept { return e == eof() ? 0 : e; }
  static constexpr UC to_char_type(int_type e) noexcept { return UC{static_cast<unsigned char>(e), 0}; }
  static constexpr int_type to_int_type(UC c) noexcept { return c.c; }
  static constexpr bool eq_int_type(int_type a, int_type b) noexcept { return a == b; }
  static constexpr int_type eof() noexcept { return -1; }
};

using Str = std::basic_string<UC, UTraits>;
using SV = std::basic_string_view<UC, UTraits>;
constexpr std::size_t npos = Str::npos;

// a string of the letters of word, each tagged by its position mixed with salt
static Str make(const char* word, unsigned salt) {
  Str s;
  for (unsigned i = 0; word[i]; ++i) s.push_back(UC{static_cast<unsigned char>(word[i]), static_cast<unsigned char>((i * 7 + salt) % 5 + 1)});
  return s;
}

// [string.view.find] definitions, transcribed
static bool eqv(UC a, UC b) { return UTraits::eq(a, b); }
static std::size_t o_find(SV h, SV n, std::size_t pos) {
  for (std::size_t x = pos; x <= h.size() && n.size() <= h.size() - x; ++x) {
    bool ok = true;
    for (std::size_t i = 0; i < n.size(); ++i) ok = ok && eqv(h[x + i], n[i]);
    if (ok) return x;
  }
  return npos;
}
static std::size_t o_rfind(SV h, SV n, std::size_t pos) {
  if (n.size() > h.size()) return npos;
  std::size_t x = std::min(pos, h.size() - n.size());
  for (;; --x) {
    bool ok = true;
    for (std::size_t i = 0; i < n.size(); ++i) ok = ok && eqv(h[x + i], n[i]);
    if (ok) return x;
    if (x == 0) return npos;
  }
}
static bool in(SV set, UC c) {
  for (UC d : set)
    if (eqv(c, d)) return true;
  return false;
}
static std::size_t o_ffo(SV h, SV n, std::size_t pos, bool want) {
  for (std::size_t x = pos; x < h.size(); ++x)
    if (in(n, h[x]) == want) return x;
  return npos;
}
static std::size_t o_flo(SV h, SV n, std::size_t pos, bool want) {
  if (h.empty()) return npos;
  for (std::size_t x = std::min(pos, h.size() - 1);; --x) {
    if (in(n, h[x]) == want) return x;
    if (x == 0) return npos;
  }
}

static const char* const words[] = {"", "a", "b", "A", "ab", "ba", "aa", "abA", "aab", "bab", "abab", "aabba", "babab"};

static void searches() {
  for (const char* hw : words)
    for (const char* nw : words)
      for (unsigned salt : {0u, 3u}) {
        Str h = make(hw, 0), n = make(nw, salt);
        SV hv(h), nv(n);
        for (std::size_t pos : {std::size_t{0}, std::size_t{1}, std::size_t{2}, std::size_t{4}, std::size_t{6}, npos}) {
          CHECK(h.find(n, pos) == o_find(hv, nv, pos));
          CHECK(hv.find(nv, pos) == o_find(hv, nv, pos));
          CHECK(h.find(n.c_str(), pos) == o_find(hv, nv, pos));
          CHECK(h.find(n.data(), pos, n.size()) == o_find(hv, nv, pos));
          CHECK(h.rfind(n, pos) == o_rfind(hv, nv, pos));
          CHECK(hv.rfind(nv, pos) == o_rfind(hv, nv, pos));
          CHECK(h.find_first_of(n, pos) == o_ffo(hv, nv, pos, true));
          CHECK(hv.find_first_of(nv, pos) == o_ffo(hv, nv, pos, true));
          CHECK(h.find_first_not_of(n, pos) == o_ffo(hv, nv, pos, false));
          CHECK(hv.find_first_not_of(nv, pos) == o_ffo(hv, nv, pos, false));
          CHECK(h.find_last_of(n, pos) == o_flo(hv, nv, pos, true));
          CHECK(hv.find_last_of(nv, pos) == o_flo(hv, nv, pos, true));
          CHECK(h.find_last_not_of(n, pos) == o_flo(hv, nv, pos, false));
          CHECK(hv.find_last_not_of(nv, pos) == o_flo(hv, nv, pos, false));
          if (n.size() == 1) {
            UC c = n[0];
            CHECK(h.find(c, pos) == o_find(hv, nv, pos));
            CHECK(hv.find(c, pos) == o_find(hv, nv, pos));
            CHECK(h.rfind(c, pos) == o_rfind(hv, nv, pos));
            CHECK(h.find_first_of(c, pos) == o_ffo(hv, nv, pos, true));
            CHECK(h.find_first_not_of(c, pos) == o_ffo(hv, nv, pos, false));
            CHECK(h.find_last_of(c, pos) == o_flo(hv, nv, pos, true));
            CHECK(h.find_last_not_of(c, pos) == o_flo(hv, nv, pos, false));
          }
        }
        // comparisons: by letters only
        Str hs = make(hw, 1);
        Str ns = make(nw, 4);
        std::string a(hw), b(nw);
        int expect = a.compare(b);
        expect = expect < 0 ? -1 : expect > 0 ? 1 : 0;
        int got = hs.compare(ns);
        CHECK((got < 0 ? -1 : got > 0 ? 1 : 0) == expect);
        got = SV(hs).compare(SV(ns));
        CHECK((got < 0 ? -1 : got > 0 ? 1 : 0) == expect);
        CHECK((hs == ns) == (a == b));
        CHECK((SV(hs) == SV(ns)) == (a == b));
        CHECK((hs == ns.c_str()) == (a == b));
        static_assert(std::is_same_v<decltype(hs <=> ns), std::weak_ordering>);
        static_assert(std::is_same_v<decltype(SV(hs) <=> SV(ns)), std::weak_ordering>);
        CHECK((hs <=> ns) == (a <=> b));
        CHECK((SV(hs) <=> SV(ns)) == (a <=> b));
        CHECK(hs.starts_with(SV(ns)) == a.starts_with(b));
        CHECK(hs.ends_with(SV(ns)) == a.ends_with(b));
        CHECK(hs.contains(SV(ns)) == a.contains(b));
        CHECK(SV(hs).starts_with(SV(ns)) == a.starts_with(b));
        CHECK(SV(hs).ends_with(ns.c_str()) == a.ends_with(b));
        CHECK(SV(hs).contains(SV(ns)) == a.contains(b));
      }
}

static void lengths_and_copy() {
  // the first character with c == 0 ends a pointer argument, whatever its tag
  UC arr[] = {{'x', 1}, {'y', 2}, {0, 9}, {'z', 3}, {0, 0}};
  Str s(arr);
  CHECK(s.size() == 2);
  SV v(arr);
  CHECK(v.size() == 2);
  Str t;
  t.assign(arr);
  CHECK(t.size() == 2);
  t.append(arr);
  CHECK(t.size() == 4);
  t.insert(0, arr);
  CHECK(t.size() == 6);
  CHECK(t.find(arr) == 0);
  CHECK(t.compare(arr) > 0);
  CHECK(Str(arr + 3).size() == 1);
  // c_str()'s terminator is charT(): {0, 0}
  CHECK(s.c_str()[2].c == 0 && s.c_str()[2].tag == 0);
  // tags are kept by the copies
  CHECK(s[1].tag == 2 && t[5].tag == 2);

  UC out[8] = {};
  int before = copy_calls;
  CHECK(s.copy(out, 8) == 2);
  CHECK(copy_calls == before + 1);
  CHECK(out[0].c == 'x' && out[0].tag == 1 && out[1].c == 'y');
  before = copy_calls;
  CHECK(v.copy(out, 1, 1) == 1);
  CHECK(copy_calls == before + 1);
  CHECK(out[0].c == 'y' && out[0].tag == 2);
}

int main() {
  searches();
  lengths_and_copy();
}
