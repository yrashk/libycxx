// [string.require]/1: "If any operation would cause size() to exceed max_size(), that
// operation throws an exception object of type length_error." [string.insert]/10,14 and
// [string.replace]/8,12 restate it; [string.capacity]/15 for reserve. /2: a throwing
// member has no other effect on the string.
// REQUIRES: exceptions
#include <string>
#include <stdexcept>
#include "check.hpp"

template <class F>
bool length_error_and_unchanged(std::string& s, F f) {
  const std::string before = s;
  bool threw = false;
  try {
    f();
  } catch (const std::length_error&) {
    threw = true;
  } catch (...) {
  }
  return threw && s == before;
}

int main() {
  std::string s = "abc";
  const auto max = s.max_size();
  CHECK(max >= s.size());
  CHECK(length_error_and_unchanged(s, [&] { s.append(max, 'x'); }));
  CHECK(length_error_and_unchanged(s, [&] { s.append(max - 1, 'x'); }));
  CHECK(length_error_and_unchanged(s, [&] { s.insert(0, max, 'x'); }));
  CHECK(length_error_and_unchanged(s, [&] { s.insert(s.cbegin(), max, 'x'); }));
  CHECK(length_error_and_unchanged(s, [&] { s.replace(0, 1, max, 'x'); }));
  CHECK(length_error_and_unchanged(s, [&] { s.resize(max + 1 == 0 ? max : max + 1); }) ||
        max == std::string::npos);
  if (max < std::string::npos) {
    CHECK(length_error_and_unchanged(s, [&] { s.reserve(max + 1); }));
    CHECK(length_error_and_unchanged(s, [&] { std::string t(max + 1, 'x'); }));
    CHECK(length_error_and_unchanged(s, [&] { s.assign(max + 1, 'x'); }));
  }
  return 0;
}
