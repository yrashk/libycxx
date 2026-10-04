// [locale.codecvt.general]/3: "codecvt<char16_t, char8_t, mbstate_t> converts between the
// UTF-16 and UTF-8 encoding forms, and codecvt<char32_t, char8_t, mbstate_t> converts between
// the UTF-32 and UTF-8 encoding forms." [locale.codecvt.virtuals]/2-3: do_in / do_out
// convert no more than the source and destination allow, "Stops if it encounters a character
// it cannot convert. It always leaves the from_next and to_next pointers pointing one beyond
// the last element successfully converted." Table 93: ok, partial ("not all source characters
// converted"; with from_next == from_end: "additional source elements are needed before
// another destination element can be produced"), error ("encountered a character in [from,
// from_end) that cannot be converted"). /14 do_length: "(from_next - from) where from_next is
// the largest value in the range [from, from_end] such that the sequence of values in the
// range [from, from_next) represents max or fewer valid complete characters of type internT".
// /10: encoding() is 0 for a variable-width encoding; /11 always_noconv() is false.
// UTF-8 (ISO/IEC 10646 / Unicode 3.9): overlong forms, surrogate code points (U+D800-U+DFFF),
// code points above U+10FFFF, bytes F5-FF and stray continuation bytes are ill-formed; UTF-16
// pairs a high surrogate with a following low surrogate; UTF-32 values that are surrogates or
// above 0x10FFFF are ill-formed.
#include <locale>
#include <cwchar>
#include <iterator>
#include <string>
#include "check.hpp"

using C32 = std::codecvt<char32_t, char8_t, std::mbstate_t>;
using C16 = std::codecvt<char16_t, char8_t, std::mbstate_t>;
using R = std::codecvt_base::result;

template <class F, class I>
static R in(const F& f, std::u8string_view src, std::basic_string<I>& out, std::size_t room, std::size_t* consumed) {
  std::mbstate_t st{};
  I buf[64] = {};
  const char8_t* fn = nullptr;
  I* tn = nullptr;
  R r = f.in(st, src.data(), src.data() + src.size(), fn, buf, buf + room, tn);
  out.assign(buf, tn);
  *consumed = static_cast<std::size_t>(fn - src.data());
  return r;
}

template <class F, class I>
static R out(const F& f, std::basic_string_view<I> src, std::u8string& o, std::size_t room, std::size_t* consumed) {
  std::mbstate_t st{};
  char8_t buf[64] = {};
  const I* fn = nullptr;
  char8_t* tn = nullptr;
  R r = f.out(st, src.data(), src.data() + src.size(), fn, buf, buf + room, tn);
  o.assign(buf, tn);
  *consumed = static_cast<std::size_t>(fn - src.data());
  return r;
}

int main() {
  const std::locale& loc = std::locale::classic();
  const C32& c32 = std::use_facet<C32>(loc);
  const C16& c16 = std::use_facet<C16>(loc);
  std::u32string s32;
  std::u16string s16;
  std::u8string s8;
  std::size_t n;

  CHECK(c32.encoding() == 0 && !c32.always_noconv() && c32.max_length() == 4);
  CHECK(c16.encoding() == 0 && !c16.always_noconv());

  // Boundaries of each sequence length.
  const std::u32string cps = {0x0, 0x7F, 0x80, 0x7FF, 0x800, 0xFFFF, 0x10000, 0x10FFFF, 0xD7FF, 0xE000};
  const unsigned char enc_bytes[] = {0x00, 0x7F, 0xC2, 0x80, 0xDF, 0xBF, 0xE0, 0xA0, 0x80, 0xEF, 0xBF, 0xBF,
                                     0xF0, 0x90, 0x80, 0x80, 0xF4, 0x8F, 0xBF, 0xBF, 0xED, 0x9F, 0xBF, 0xEE, 0x80, 0x80};
  const std::u8string enc(std::begin(enc_bytes), std::end(enc_bytes));
  CHECK(out(c32, std::u32string_view(cps), s8, 64, &n) == R::ok && n == cps.size() && s8 == enc);
  CHECK(in(c32, enc, s32, 64, &n) == R::ok && n == enc.size() && s32 == cps);

  // UTF-32 values that cannot be encoded.
  for (char32_t bad : {char32_t(0xD800), char32_t(0xDFFF), char32_t(0x110000), char32_t(0xFFFFFFFF)}) {
    const char32_t src[] = {U'a', bad, U'b'};
    CHECK(out(c32, std::u32string_view(src, 3), s8, 64, &n) == R::error && n == 1 && s8 == u8"a");
  }
  // Destination too small for the next complete sequence: partial, nothing half-written.
  {
    const std::u32string src = U"a\U0001F600";
    CHECK(out(c32, std::u32string_view(src), s8, 4, &n) == R::partial && n == 1 && s8 == u8"a");
    CHECK(out(c32, std::u32string_view(src), s8, 5, &n) == R::ok && n == 2);
  }

  // Ill-formed UTF-8.
  const std::u8string bad8[] = {
      u8"a\xC0\x80",          // overlong NUL
      u8"a\xC1\xBF",          // overlong
      u8"a\xE0\x80\x80",      // overlong 3-byte
      u8"a\xE0\x9F\xBF",      // overlong 3-byte (U+07FF)
      u8"a\xF0\x8F\xBF\xBF",  // overlong 4-byte (U+FFFF)
      u8"a\xED\xA0\x80",      // U+D800
      u8"a\xED\xBF\xBF",      // U+DFFF
      u8"a\xF4\x90\x80\x80",  // U+110000
      u8"a\xF5\x80\x80\x80",  // F5
      u8"a\xFF",              // FF
      u8"a\x80",              // stray continuation
      u8"a\xC3\x28",          // lead byte without a continuation
      u8"a\xE2\x82\x28",      // second continuation missing
  };
  for (const auto& b : bad8) {
    CHECK(in(c32, b, s32, 64, &n) == R::error && n == 1 && s32 == U"a");
    CHECK(in(c16, b, s16, 64, &n) == R::error && n == 1 && s16 == u"a");
  }
  // Incomplete sequence at the end of the source: partial, from_next at its first byte.
  {
    const std::u8string incs[] = {u8"a\xE2\x82", u8"a\xF0\x9F\x98", u8"a\xF0", u8"a\xC3"};
    for (const auto& i : incs) {
      CHECK(in(c32, i, s32, 64, &n) == R::partial && n == 1 && s32 == U"a");
      CHECK(in(c16, i, s16, 64, &n) == R::partial && n == 1 && s16 == u"a");
    }
  }

  // UTF-16: supplementary characters use a surrogate pair.
  {
    const std::u8string src = u8"x\U0001F600y";
    CHECK(in(c16, src, s16, 64, &n) == R::ok && n == src.size() && s16 == u"x\U0001F600y");
    CHECK(s16.size() == 4);
    // Room for one char16_t after 'x': the pair does not fit. Either the conversion stops
    // after 'x' (partial), or -- /4 Note 1: "As a result of operations on state, it can
    // return ok or partial and set from_next == from and to_next != to" -- the high surrogate
    // is stored and remembered in the state, and the next call (same state) stores the low one.
    {
      std::mbstate_t st{};
      char16_t b[4] = {};
      const char8_t* fn = nullptr;
      char16_t* tn = nullptr;
      R r = c16.in(st, src.data(), src.data() + src.size(), fn, b, b + 2, tn);
      CHECK(r != R::error && fn == src.data() + 1 && b[0] == u'x');
      if (tn == b + 2) {
        CHECK(b[1] == char16_t(0xD83D));
        const char8_t* fn2 = nullptr;
        char16_t* tn2 = nullptr;
        r = c16.in(st, fn, src.data() + src.size(), fn2, tn, b + 4, tn2);
        CHECK(r == R::ok && fn2 == src.data() + src.size() && tn2 == b + 4);
        CHECK(b[2] == char16_t(0xDE00) && b[3] == u'y');
      } else {
        CHECK(r == R::partial && tn == b + 1);
      }
    }
    CHECK(in(c16, src, s16, 3, &n) == R::partial && n == 5 && s16 == u"x\U0001F600");
    const std::u16string u = u"x\U0001F600y";
    CHECK(out(c16, std::u16string_view(u), s8, 64, &n) == R::ok && n == 4 && s8 == src);
    // Room for 3 more bytes after 'x': the 4-byte sequence does not fit. The high surrogate
    // may have been taken into the state (/4 and Note 1); the next call with the same state
    // then produces the rest.
    {
      std::mbstate_t st{};
      char8_t b[8] = {};
      const char16_t* fn = nullptr;
      char8_t* tn = nullptr;
      R r = c16.out(st, u.data(), u.data() + u.size(), fn, b, b + 4, tn);
      CHECK(r == R::partial && tn == b + 1 && b[0] == u8'x' && (fn == u.data() + 1 || fn == u.data() + 2));
      const char16_t* fn2 = nullptr;
      char8_t* tn2 = nullptr;
      r = c16.out(st, fn, u.data() + u.size(), fn2, tn, b + 8, tn2);
      CHECK(r == R::ok && fn2 == u.data() + u.size() && std::u8string(b, tn2) == src);
    }
  }
  // Unpaired surrogates in UTF-16.
  {
    const char16_t lone_low[] = {u'a', char16_t(0xDC00), u'b'};
    CHECK(out(c16, std::u16string_view(lone_low, 3), s8, 64, &n) == R::error && n == 1 && s8 == u8"a");
    // The character that cannot be converted is the unpaired high surrogate: from_next is
    // "one beyond the last element successfully converted", i.e. at it.
    const char16_t high_then_bmp[] = {u'a', char16_t(0xD800), u'b'};
    CHECK(out(c16, std::u16string_view(high_then_bmp, 3), s8, 64, &n) == R::error && n == 1);
    // A high surrogate at the end: more source is needed (partial, Table 93), or -- when the
    // facet keeps it in the state (/4) -- the sequence cannot be terminated in that state:
    // do_unshift (Table 94) must not claim ok ("completed the sequence") or noconv ("no
    // termination is needed"), which would silently drop the character.
    {
      const char16_t high_at_end[] = {u'a', char16_t(0xD83D)};
      std::mbstate_t st{};
      char8_t b[8] = {};
      const char16_t* fn = nullptr;
      char8_t* tn = nullptr;
      R r = c16.out(st, high_at_end, high_at_end + 2, fn, b, b + 8, tn);
      CHECK(tn == b + 1 && b[0] == u8'a');
      if (r == R::partial) {
        CHECK(fn == high_at_end + 1);
      } else {
        CHECK(r == R::ok && fn == high_at_end + 2);
        char8_t* un = nullptr;
        R u = c16.unshift(st, tn, b + 8, un);
        CHECK(u != R::ok && u != R::noconv);
      }
    }
  }

  // do_length counts complete characters of type internT.
  {
    std::mbstate_t st{};
    const std::u8string src = u8"a\xC3\xA9\xE2\x82\xAC\U0001F600";  // 1 + 2 + 3 + 4 bytes
    const char8_t* b = src.data();
    const char8_t* e = b + src.size();
    CHECK(c32.length(st, b, e, 0) == 0);
    CHECK(c32.length(st, b, e, 1) == 1);
    CHECK(c32.length(st, b, e, 3) == 6);
    CHECK(c32.length(st, b, e, 4) == 10);
    CHECK(c32.length(st, b, e, 100) == 10);
    CHECK(c16.length(st, b, e, 3) == 6);
    CHECK(c16.length(st, b, e, 4) == 6);   // the pair needs two char16_t
    CHECK(c16.length(st, b, e, 5) == 10);
    CHECK(c32.length(st, b, b + 8, 100) == 6);  // incomplete last character
    const std::u8string withbad = u8"ab\xC0\x80" "c";
    CHECK(c32.length(st, withbad.data(), withbad.data() + withbad.size(), 100) == 2);
  }
  return 0;
}
