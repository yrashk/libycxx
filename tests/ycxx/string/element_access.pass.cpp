// [string.access]: operator[](pos) returns data()[pos] for pos <= size() (pos == size() gives
// the null terminator); at(pos) returns operator[](pos) and throws out_of_range if pos >=
// size(); front() is operator[](0), back() is operator[](size() - 1).
// [string.accessors]: c_str() and data() return to_address(begin()); data() non-const
// returns charT*.
#include <string>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::declval<std::string&>()[0]), char&>);
static_assert(std::is_same_v<decltype(std::declval<const std::string&>()[0]), const char&>);
static_assert(std::is_same_v<decltype(std::declval<std::string&>().at(0)), char&>);
static_assert(std::is_same_v<decltype(std::declval<const std::string&>().at(0)), const char&>);
static_assert(std::is_same_v<decltype(std::declval<std::string&>().front()), char&>);
static_assert(std::is_same_v<decltype(std::declval<const std::string&>().back()), const char&>);
static_assert(std::is_same_v<decltype(std::declval<std::string&>().data()), char*>);
static_assert(std::is_same_v<decltype(std::declval<const std::string&>().data()), const char*>);
static_assert(std::is_same_v<decltype(std::declval<std::string&>().c_str()), const char*>);
static_assert(noexcept(std::declval<std::string&>().data()));
static_assert(noexcept(std::declval<const std::string&>().c_str()));

constexpr bool test() {
  std::string s = "abc";
  const std::string& cs = s;
  if (s[0] != 'a' || cs[2] != 'c' || cs[3] != '\0') return false;
  if (&s[1] != s.data() + 1) return false;
  s[1] = 'B';
  s.at(2) = 'C';
  s.front() = 'A';
  if (s != "ABC") return false;
  if (cs.at(0) != 'A' || cs.front() != 'A' || cs.back() != 'C') return false;
  s.back() = 'Z';
  if (s != "ABZ") return false;
  if (s.data() != std::to_address(s.begin()) || s.c_str() != s.data()) return false;
  s.data()[0] = 'x';
  if (s != "xBZ") return false;
  std::string e;
  if (e[0] != '\0' || e.data()[0] != '\0') return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::string s = "abc";
  const std::string& cs = s;
  int threw = 0;
  try { (void)s.at(3); } catch (const std::out_of_range&) { ++threw; }
  try { (void)cs.at(3); } catch (const std::out_of_range&) { ++threw; }
  try { (void)std::string().at(0); } catch (const std::out_of_range&) { ++threw; }
  CHECK(threw == 3);
  return 0;
}
