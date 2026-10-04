// [string.conversions]/4-6: stof, stod, stold call strtof / strtod / strtold on
// str.c_str(); idx receives the index of the first unconverted character; invalid_argument
// when no conversion can be performed; out_of_range when the function sets errno to ERANGE
// or the value is not representable.
#include <string>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::stof(std::string())), float>);
static_assert(std::is_same_v<decltype(std::stod(std::string())), double>);
static_assert(std::is_same_v<decltype(std::stold(std::string())), long double>);

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
  CHECK(std::stof("1.5") == 1.5f);
  CHECK(std::stod("  -2.25e2rest", &idx) == -225.0 && idx == 9);
  CHECK(std::stold("0x1p4", &idx) == 16.0L && idx == 5);
  CHECK(std::stod("inf") > 1e308);
  double n = std::stod("nan", &idx);
  CHECK(n != n && idx == 3);
  CHECK(std::stod(".5") == 0.5);
  CHECK(std::stof("3", &idx) == 3.0f && idx == 1);

  CHECK(which_throw([] { (void)std::stof(""); }) == 1);
  CHECK(which_throw([] { (void)std::stod("e5"); }) == 1);
  CHECK(which_throw([] { (void)std::stold("  x"); }) == 1);
  CHECK(which_throw([] { (void)std::stod("1e999"); }) == 2);
  CHECK(which_throw([] { (void)std::stof("1e100"); }) == 2);
  CHECK(which_throw([] { (void)std::stod("-1e999"); }) == 2);
  return 0;
}
