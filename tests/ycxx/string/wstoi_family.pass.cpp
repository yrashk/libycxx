// [string.conversions]/8-13: the wstring overloads of stoi, stol, stoul, stoll, stoull,
// stof, stod, stold call wcstol / wcstoul / wcstoll / wcstoull / wcstof / wcstod / wcstold,
// store the first unconverted index in *idx, throw invalid_argument when nothing converts
// and out_of_range when the value is out of range.
// REQUIRES: exceptions
#include <string>
#include <climits>
#include <cstddef>
#include <stdexcept>
#include "check.hpp"

template <class F>
int which_throw(F f) {
  try {
    f();
  } catch (const std::invalid_argument&) {
    return 1;
  } catch (const std::out_of_range&) {
    return 2;
  } catch (...) {
    return 3;
  }
  return 0;
}

int main() {
  std::size_t idx = 0;
  CHECK(std::stoi(std::wstring(L" 12ab"), &idx) == 12 && idx == 3);
  CHECK(std::stoi(std::wstring(L"ab"), nullptr, 16) == 171);
  CHECK(std::stol(std::wstring(L"-5")) == -5L);
  CHECK(std::stoul(std::wstring(L"0x10"), &idx, 0) == 16UL && idx == 4);
  CHECK(std::stoll(std::wstring(L"-9223372036854775808")) == LLONG_MIN);
  CHECK(std::stoull(std::wstring(L"11"), nullptr, 2) == 3ULL);
  CHECK(std::stof(std::wstring(L"0.25")) == 0.25f);
  CHECK(std::stod(std::wstring(L"1e3x"), &idx) == 1000.0 && idx == 3);
  CHECK(std::stold(std::wstring(L"-0.5")) == -0.5L);
  CHECK(which_throw([] { (void)std::stoi(std::wstring(L"")); }) == 1);
  CHECK(which_throw([] { (void)std::stod(std::wstring(L"x")); }) == 1);
  CHECK(which_throw([] { (void)std::stoll(std::wstring(L"99999999999999999999999")); }) == 2);
  CHECK(which_throw([] { (void)std::stod(std::wstring(L"1e999")); }) == 2);
  if (sizeof(long) > sizeof(int))
    CHECK(which_throw([] { (void)std::stoi(std::wstring(L"4294967296")); }) == 2);
  return 0;
}
