// [string.conversions]/1-3: stoi, stol, stoul, stoll, stoull call strtol / strtoul / strtoll /
// strtoull on str.c_str() with the given base; when idx != nullptr store the index of the
// first unconverted character; throw invalid_argument when no conversion can be performed
// and out_of_range on ERANGE or when the value does not fit the return type.
// REQUIRES: exceptions
#include <string>
#include <climits>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::stoi(std::string())), int>);
static_assert(std::is_same_v<decltype(std::stol(std::string())), long>);
static_assert(std::is_same_v<decltype(std::stoul(std::string())), unsigned long>);
static_assert(std::is_same_v<decltype(std::stoll(std::string())), long long>);
static_assert(std::is_same_v<decltype(std::stoull(std::string())), unsigned long long>);

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
  std::size_t idx = 99;
  CHECK(std::stoi("42") == 42);
  CHECK(std::stoi("  -17xyz", &idx) == -17 && idx == 5);
  CHECK(std::stoi("+8", &idx) == 8 && idx == 2);
  CHECK(std::stoi("ff", &idx, 16) == 255 && idx == 2);
  CHECK(std::stoi("0x1A", nullptr, 16) == 26);
  CHECK(std::stoi("0x1A", &idx, 0) == 26 && idx == 4);
  CHECK(std::stoi("017", nullptr, 0) == 15);
  CHECK(std::stoi("101", nullptr, 2) == 5);
  CHECK(std::stoi("z", nullptr, 36) == 35);
  CHECK(std::stol("-2147483648") == -2147483647L - 1);
  CHECK(std::stoul("4294967295") == 4294967295UL);
  CHECK(std::stoll("-9223372036854775808") == LLONG_MIN);
  CHECK(std::stoull("18446744073709551615") == ULLONG_MAX);
  CHECK(std::stoull("777", &idx, 8) == 511 && idx == 3);

  CHECK(which_throw([] { (void)std::stoi(""); }) == 1);
  CHECK(which_throw([] { (void)std::stoi("abc"); }) == 1);
  CHECK(which_throw([] { (void)std::stoi("   "); }) == 1);
  CHECK(which_throw([] { (void)std::stol("-"); }) == 1);
  CHECK(which_throw([] { (void)std::stoul("x1"); }) == 1);
  CHECK(which_throw([] { (void)std::stoll("9", nullptr, 8); }) == 1);
  CHECK(which_throw([] { (void)std::stoull(""); }) == 1);
  // ERANGE from the C function
  CHECK(which_throw([] { (void)std::stoll("99999999999999999999999"); }) == 2);
  CHECK(which_throw([] { (void)std::stoull("99999999999999999999999"); }) == 2);
  CHECK(which_throw([] { (void)std::stol("-99999999999999999999999"); }) == 2);
  // representable as long but not as int
  if (sizeof(long) > sizeof(int)) {
    CHECK(which_throw([] { (void)std::stoi("2147483648"); }) == 2);
    CHECK(which_throw([] { (void)std::stoi("-2147483649"); }) == 2);
  }
  // idx is not written when an exception is thrown
  idx = 77;
  CHECK(which_throw([&] { (void)std::stoi("q", &idx); }) == 1);
  CHECK(idx == 77);
  // embedded null ends the conversion (c_str())
  CHECK(std::stoi(std::string("12\0" "34", 5), &idx) == 12 && idx == 2);
  return 0;
}
