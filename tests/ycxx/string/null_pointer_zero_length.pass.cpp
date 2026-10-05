// A null pointer with a length of zero is a valid range ([iterator.requirements.general]/8:
// "a range [i, i + 0) is an empty range"; a null pointer value plus 0 is the null pointer
// value, [expr.add]/4.1), and the (pointer, length) operations of char_traits, basic_string and
// basic_string_view whose only precondition is that "[s, s + n) is a valid range" accept it.
// The library must not pass such a pointer to a C function whose arguments must not be null
// (memcpy, memmove, memcmp, memchr, memset, wmemcpy, ... ISO C 7.26.1/2, 7.31.4/2): run under
// SANITIZER=ubsan, -fsanitize=nonnull-attribute reports any such call.
//   [char.traits.require]: compare(p,q,0) returns 0; find(p,0,c) returns nullptr (no q in
//     [p, p+0)); move(s,p,0), copy(s,p,0), assign(s,0,c) return s.
//   [string.cons]/? basic_string(const charT* s, size_type n): "Preconditions: [s, s + n) is a
//     valid range"; append/assign/insert/replace/compare/find with (s, n) likewise; [string.copy]
//     copy(s, n, pos) writes min(n, size() - pos) characters.
//   [string.view.cons]: basic_string_view(const charT* str, size_type len): "Preconditions:
//     [str, str + len) is a valid range"; the default constructor has data() == nullptr.
#include <cstring>
#include <cwchar>
#include <string>
#include <string_view>
#include "check.hpp"

template <class C>
void traits_case() {
  using T = std::char_traits<C>;
  C* null = nullptr;
  const C* cnull = nullptr;
  CHECK(T::compare(cnull, cnull, 0) == 0);
  C buf[1] = {C('x')};
  CHECK(T::compare(buf, cnull, 0) == 0);
  CHECK(T::find(cnull, 0, C('a')) == nullptr);
  CHECK(T::find(buf, 0, C('x')) == nullptr);
  CHECK(T::move(null, cnull, 0) == nullptr);
  CHECK(T::copy(null, cnull, 0) == nullptr);
  CHECK(T::assign(null, 0, C('a')) == nullptr);
  CHECK(T::move(buf, cnull, 0) == buf && buf[0] == C('x'));
  CHECK(T::copy(buf, cnull, 0) == buf && buf[0] == C('x'));
}

template <class C>
void string_case() {
  using S = std::basic_string<C>;
  using SV = std::basic_string_view<C>;
  const C* cnull = nullptr;
  const C abc[] = {C('a'), C('b'), C('c'), C(0)};

  S s(cnull, 0);
  CHECK(s.empty() && s.c_str()[0] == C(0));
  S t(abc);
  CHECK(&t.append(cnull, 0) == &t && t == abc);
  CHECK(&t.assign(abc, 3) == &t);
  CHECK(&t.insert(1, cnull, 0) == &t && t == abc);
  CHECK(&t.replace(1, 1, cnull, 0) == &t && t.size() == 2 && t[0] == C('a') && t[1] == C('c'));
  t = abc;
  CHECK(&t.replace(0, 0, cnull, 0) == &t && t == abc);
  CHECK(t.compare(0, 0, cnull, 0) == 0);
  CHECK(t.compare(0, 3, cnull, 0) > 0);
  CHECK(S().compare(0, 0, cnull, 0) == 0);
  CHECK(t.find(cnull, 0, 0) == 0);
  CHECK(t.find(cnull, 3, 0) == 3);
  CHECK(t.find(cnull, 4, 0) == S::npos);
  CHECK(t.rfind(cnull, S::npos, 0) == 3);
  CHECK(t.find_first_of(cnull, 0, 0) == S::npos);
  CHECK(t.find_last_of(cnull, S::npos, 0) == S::npos);
  CHECK(t.find_first_not_of(cnull, 0, 0) == 0);
  CHECK(t.find_last_not_of(cnull, S::npos, 0) == 2);
  CHECK(S().find(cnull, 0, 0) == 0);
  CHECK(t.copy(nullptr, 0, 1) == 0);
  CHECK(S().copy(nullptr, 5) == 0);
  CHECK(&t.assign(cnull, 0) == &t && t.empty());

  // Iterator-pair forms with two null pointers (an empty range of C*).
  S r(cnull, cnull);
  CHECK(r.empty());
  r = abc;
  r.append(cnull, cnull);
  r.insert(r.begin(), cnull, cnull);
  r.replace(r.begin(), r.begin(), cnull, cnull);
  CHECK(r == abc);
  r.assign(cnull, cnull);
  CHECK(r.empty());

  SV v;
  CHECK(v.data() == nullptr && v.size() == 0);
  SV w(cnull, 0);
  CHECK(w.empty() && v == w && !(v < w) && v.compare(w) == 0);
  CHECK(v.compare(SV(abc)) < 0 && SV(abc).compare(v) > 0);
  CHECK(v.find(v) == 0 && v.rfind(v) == 0 && v.find(C('a')) == SV::npos && v.rfind(C('a')) == SV::npos);
  CHECK(SV(abc).find(v) == 0 && SV(abc).rfind(v) == 3);
  CHECK(v.find_first_of(abc) == SV::npos && SV(abc).find_first_of(v) == SV::npos);
  CHECK(v.starts_with(v) && v.ends_with(v) && SV(abc).starts_with(v) && SV(abc).ends_with(v) && SV(abc).contains(v));
  CHECK(!v.starts_with(C('a')) && !v.contains(C('a')));
  CHECK(v.copy(nullptr, 3) == 0);
  CHECK(v.substr().data() == nullptr || v.substr().empty());
  S from_view(v);
  CHECK(from_view.empty());
  S u(abc);
  u += v;
  u.append(v);
  u.insert(0, v);
  u.replace(0, 0, v);
  CHECK(u == abc && u.compare(v) > 0 && u.find(v) == 0);
  u = v;
  CHECK(u.empty());
  CHECK(std::hash<SV>{}(v) == std::hash<SV>{}(SV(abc, 0)));
  CHECK(std::hash<S>{}(S()) == std::hash<SV>{}(v));
}

int main() {
  traits_case<char>();
  traits_case<wchar_t>();
  traits_case<char8_t>();
  traits_case<char16_t>();
  traits_case<char32_t>();
  string_case<char>();
  string_case<wchar_t>();
  string_case<char8_t>();
  string_case<char16_t>();
  string_case<char32_t>();
  return 0;
}
