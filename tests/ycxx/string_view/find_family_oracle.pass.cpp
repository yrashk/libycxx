// [string.view.find]: every member of the find family, checked against the draft's own
// definitions for every pos in [0, size() + 2] and pos == npos, with empty needles, needles
// longer than the view, and empty views. For each function the draft says "Let xpos be the
// lowest/highest position, if possible, such that the following conditions hold":
//   find:              pos <= xpos and xpos + str.size() <= size(), and traits::eq on
//                      every character of str
//   rfind:             xpos <= pos and xpos + str.size() <= size(), same match
//   find_first_of:     pos <= xpos < size(), traits::eq(data_[xpos], str[I]) for some I
//   find_last_of:      xpos <= pos, xpos < size(), same
//   find_first_not_of: pos <= xpos < size(), traits::eq(data_[xpos], str[I]) for no I
//   find_last_not_of:  xpos <= pos, xpos < size(), same
// "Returns: xpos if the function can determine such a value for xpos. Otherwise, returns
// npos." The charT overload is the one-character needle; (s, pos, n) uses
// basic_string_view(s, n), so s == nullptr with n == 0 is a valid empty needle; (s, pos)
// uses basic_string_view(s).
#include <string_view>
#include <cstddef>
#include "check.hpp"

template <class C>
struct Oracle {
  using SV = std::basic_string_view<C>;
  static constexpr std::size_t npos = SV::npos;
  static constexpr bool match_at(SV h, SV n, std::size_t x) {
    for (std::size_t i = 0; i < n.size(); ++i)
      if (h[x + i] != n[i]) return false;
    return true;
  }
  static constexpr bool in(SV n, C c) {
    for (C d : n)
      if (d == c) return true;
    return false;
  }
  static constexpr std::size_t find(SV h, SV n, std::size_t pos) {
    for (std::size_t x = pos; x <= h.size() && x + n.size() <= h.size(); ++x)
      if (match_at(h, n, x)) return x;
    return npos;
  }
  static constexpr std::size_t rfind(SV h, SV n, std::size_t pos) {
    if (n.size() > h.size()) return npos;
    std::size_t x = h.size() - n.size();
    if (pos < x) x = pos;
    for (;; --x) {
      if (match_at(h, n, x)) return x;
      if (x == 0) return npos;
    }
  }
  static constexpr std::size_t first_of(SV h, SV n, std::size_t pos, bool want) {
    for (std::size_t x = pos; x < h.size(); ++x)
      if (in(n, h[x]) == want) return x;
    return npos;
  }
  static constexpr std::size_t last_of(SV h, SV n, std::size_t pos, bool want) {
    if (h.empty()) return npos;
    std::size_t x = pos < h.size() ? pos : h.size() - 1;
    for (;; --x) {
      if (in(n, h[x]) == want) return x;
      if (x == 0) return npos;
    }
  }
};

template <class C>
constexpr bool run(std::basic_string_view<C> h, std::basic_string_view<C> n) {
  using O = Oracle<C>;
  using SV = std::basic_string_view<C>;
  const std::size_t npos = SV::npos;
  // a null-terminated copy of the needle for the (const charT*, pos) overloads
  C z[9] = {};
  for (std::size_t i = 0; i < n.size(); ++i) z[i] = n[i];
  const C* np = n.empty() ? nullptr : n.data();  // nullptr with n == 0 is a valid range
  for (std::size_t k = 0; k <= h.size() + 3; ++k) {
    const std::size_t pos = k == h.size() + 3 ? npos : k;
    if (h.find(n, pos) != O::find(h, n, pos)) return false;
    if (h.find(np, pos, n.size()) != O::find(h, n, pos)) return false;
    if (h.find(z, pos) != O::find(h, n, pos)) return false;
    if (h.rfind(n, pos) != O::rfind(h, n, pos)) return false;
    if (h.rfind(np, pos, n.size()) != O::rfind(h, n, pos)) return false;
    if (h.rfind(z, pos) != O::rfind(h, n, pos)) return false;
    if (h.find_first_of(n, pos) != O::first_of(h, n, pos, true)) return false;
    if (h.find_first_of(np, pos, n.size()) != O::first_of(h, n, pos, true)) return false;
    if (h.find_first_of(z, pos) != O::first_of(h, n, pos, true)) return false;
    if (h.find_last_of(n, pos) != O::last_of(h, n, pos, true)) return false;
    if (h.find_last_of(np, pos, n.size()) != O::last_of(h, n, pos, true)) return false;
    if (h.find_last_of(z, pos) != O::last_of(h, n, pos, true)) return false;
    if (h.find_first_not_of(n, pos) != O::first_of(h, n, pos, false)) return false;
    if (h.find_first_not_of(np, pos, n.size()) != O::first_of(h, n, pos, false)) return false;
    if (h.find_first_not_of(z, pos) != O::first_of(h, n, pos, false)) return false;
    if (h.find_last_not_of(n, pos) != O::last_of(h, n, pos, false)) return false;
    if (h.find_last_not_of(np, pos, n.size()) != O::last_of(h, n, pos, false)) return false;
    if (h.find_last_not_of(z, pos) != O::last_of(h, n, pos, false)) return false;
    if (n.size() == 1) {
      const C c = n[0];
      if (h.find(c, pos) != O::find(h, n, pos)) return false;
      if (h.rfind(c, pos) != O::rfind(h, n, pos)) return false;
      if (h.find_first_of(c, pos) != O::first_of(h, n, pos, true)) return false;
      if (h.find_last_of(c, pos) != O::last_of(h, n, pos, true)) return false;
      if (h.find_first_not_of(c, pos) != O::first_of(h, n, pos, false)) return false;
      if (h.find_last_not_of(c, pos) != O::last_of(h, n, pos, false)) return false;
    }
  }
  return true;
}

template <class C>
constexpr bool test() {
  using SV = std::basic_string_view<C>;
  const C hay[] = {C('a'), C('b'), C('a'), C('a'), C('b'), C('c'), C('a')};  // "abaabca"
  const C nd[] = {C('a'), C('b'), C('c'), C('x'), C('a'), C('a'), C('b'), C('a')};
  SV hs[] = {SV(), SV(hay, 1), SV(hay, 2), SV(hay, 7)};
  SV ns[] = {SV(),        SV(nd, 1),     SV(nd + 1, 1), SV(nd + 3, 1), SV(nd, 2),
             SV(nd + 4, 3), SV(nd + 1, 2), SV(nd, 3),     SV(nd + 2, 2), SV(nd + 4, 4),
             SV(hay, 7),  SV(nd, 8)};
  for (SV h : hs)
    for (SV n : ns)
      if (!run<C>(h, n)) return false;
  return true;
}

static_assert(test<char>());
static_assert(test<char16_t>());

int main() {
  CHECK(test<char>());
  CHECK(test<wchar_t>());
  CHECK(test<char8_t>());
  CHECK(test<char16_t>());
  CHECK(test<char32_t>());
  return 0;
}
