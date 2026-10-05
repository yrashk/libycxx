// The classic locale's codecvt<wchar_t, char, mbstate_t> ([locale.codecvt.general]: converts
// between the native narrow and wide character sets) and codecvt<char, char, mbstate_t>.
// [locale.codecvt.virtuals]/10: encoding() is -1 for a state-dependent encoding, else the
// constant number of external characters per internal character, or 0 if not constant; /15
// max_length() is the largest do_length(state, from, from_end, 1) "for any valid range", so it
// is at least 1, at least encoding() when that is positive (a constant N consumes exactly N),
// and bounds what length(…, 1) returns; /14: length(…, max) counts the external characters of
// at most max complete internal characters; /7-9 Table 94: unshift from the initial state
// either needs no termination (noconv) or completes the sequence storing at most what fits
// (ok); the same for codecvt<char, char>, whose other members are fixed: /11 always_noconv()
// is true, /14 length is min(max, from_end - from), /15 max_length() is 1.
// COUNTERPART: libcxx:localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/wchar_t_(encoding|max_length|unshift).pass.cpp
#include <cwchar>
#include <locale>
#include "check.hpp"

int main() {
  const std::locale& c = std::locale::classic();
  {
    using CV = std::codecvt<wchar_t, char, std::mbstate_t>;
    const CV& cv = std::use_facet<CV>(c);
    static_assert(noexcept(cv.encoding()) && noexcept(cv.max_length()) && noexcept(cv.always_noconv()));
    const int enc = cv.encoding();
    const int mx = cv.max_length();
    CHECK(enc >= -1);
    CHECK(mx >= 1);
    if (enc > 0) CHECK(mx == enc);

    // The basic characters each convert to a single wide character; length(…, n) counts them.
    const char text[] = "Hello, world 0123";
    const int n = sizeof text - 1;
    std::mbstate_t st{};
    const int one = cv.length(st, text, text + n, 1);
    CHECK(one >= 1 && one <= mx);
    st = std::mbstate_t{};
    CHECK(cv.length(st, text, text + n, 5) <= 5 * mx);
    st = std::mbstate_t{};
    CHECK(cv.length(st, text, text + n, 0) == 0);
    st = std::mbstate_t{};
    CHECK(cv.length(st, text, text, 3) == 0);

    // A conversion of the basic characters round-trips; then unshift.
    wchar_t wide[32];
    const char* from_next = nullptr;
    wchar_t* to_next = nullptr;
    st = std::mbstate_t{};
    CHECK(cv.in(st, text, text + n, from_next, wide, wide + 32, to_next) == CV::ok);
    CHECK(from_next == text + n && to_next - wide >= 1);
    char back[64];
    const wchar_t* wnext = nullptr;
    char* bnext = nullptr;
    std::mbstate_t ost{};
    CHECK(cv.out(ost, wide, to_next, wnext, back, back + 64, bnext) == CV::ok);
    CHECK(wnext == to_next && bnext - back == n);
    for (int i = 0; i < n; ++i) CHECK(back[i] == text[i]);
    char term[16];
    char* tnext = nullptr;
    const auto r = cv.unshift(ost, term, term + 16, tnext);
    CHECK(r == CV::noconv || (r == CV::ok && tnext >= term && tnext <= term + 16));
    std::mbstate_t init{};
    tnext = nullptr;
    const auto r0 = cv.unshift(init, term, term + 16, tnext);
    CHECK(r0 == CV::noconv || (r0 == CV::ok && tnext == term));
  }
  {
    using CV = std::codecvt<char, char, std::mbstate_t>;
    const CV& cv = std::use_facet<CV>(c);
    CHECK(cv.always_noconv());
    CHECK(cv.max_length() == 1);
    const char text[] = "abcdef";
    std::mbstate_t st{};
    CHECK(cv.length(st, text, text + 6, 4) == 4);
    CHECK(cv.length(st, text, text + 6, 9) == 6);
    char term[4];
    char* tnext = nullptr;
    const auto r = cv.unshift(st, term, term + 4, tnext);
    CHECK(r == CV::noconv || (r == CV::ok && tnext == term));
  }
}
