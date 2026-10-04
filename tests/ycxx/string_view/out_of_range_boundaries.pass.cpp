// [string.view.ops]: copy(s, n, pos), substr(pos, n), subview(pos, n): "Throws: out_of_range
// if pos > size()." compare(pos1, n1, str) etc. are substr(pos1, n1).compare(...), so they
// throw for pos1 > size() (and for pos2 > str.size() in the five-argument form).
// [string.view.access]: at(pos) "Throws: out_of_range if pos >= size()."
// The boundary: pos == size() is valid (an empty result), size() + 1 and npos throw; for every
// character type. out_of_range derives from logic_error ([out.of.range]).
#include <string_view>
#include <stdexcept>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_base_of_v<std::logic_error, std::out_of_range>);

template <class F>
int outcome(F f) {  // 0: no exception, 1: out_of_range, 2: something else
  try {
    f();
  } catch (const std::out_of_range&) {
    return 1;
  } catch (...) {
    return 2;
  }
  return 0;
}

template <class C>
bool test() {
  using SV = std::basic_string_view<C>;
  const C txt[] = {C('x'), C('y'), C('z'), C(0)};
  const SV s(txt, 3);
  const SV e;
  C buf[4] = {};
  const auto npos = SV::npos;
  bool ok = true;
  for (SV v : {s, e}) {
    const auto n = v.size();
    ok = ok && outcome([&] { (void)v.substr(n); }) == 0;
    ok = ok && outcome([&] { (void)v.substr(n, npos); }) == 0;
    ok = ok && outcome([&] { (void)v.substr(n + 1); }) == 1;
    ok = ok && outcome([&] { (void)v.substr(npos, 0); }) == 1;
    ok = ok && outcome([&] { (void)v.subview(n); }) == 0;
    ok = ok && outcome([&] { (void)v.subview(n + 1, 0); }) == 1;
    ok = ok && outcome([&] { (void)v.subview(npos); }) == 1;
    ok = ok && outcome([&] { (void)v.copy(buf, 0, n); }) == 0;
    ok = ok && outcome([&] { (void)v.copy(buf, 4, n); }) == 0;
    ok = ok && outcome([&] { (void)v.copy(buf, 0, n + 1); }) == 1;  // even when n == 0
    ok = ok && outcome([&] { (void)v.copy(buf, 1, npos); }) == 1;
    ok = ok && outcome([&] { (void)v.compare(n, 1, s); }) == 0;
    ok = ok && outcome([&] { (void)v.compare(n + 1, 0, s); }) == 1;
    ok = ok && outcome([&] { (void)v.compare(n + 1, 0, txt); }) == 1;
    ok = ok && outcome([&] { (void)v.compare(n + 1, 0, txt, 1); }) == 1;
    ok = ok && outcome([&] { (void)v.compare(0, 0, s, 3, 1); }) == 0;
    ok = ok && outcome([&] { (void)v.compare(0, 0, s, 4, 1); }) == 1;
    ok = ok && outcome([&] { (void)v.compare(n + 1, 0, s, 0, 0); }) == 1;
    ok = ok && outcome([&] { (void)v.at(n); }) == 1;
    ok = ok && outcome([&] { (void)v.at(npos); }) == 1;
  }
  ok = ok && outcome([&] { (void)s.at(2); }) == 0;
  return ok;
}

int main() {
  CHECK(test<char>());
  CHECK(test<wchar_t>());
  CHECK(test<char8_t>());
  CHECK(test<char16_t>());
  CHECK(test<char32_t>());
  return 0;
}
