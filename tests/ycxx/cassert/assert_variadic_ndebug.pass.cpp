// [assertions.assert]: assert(...) is variadic, so an argument with unparenthesised commas
// (template argument lists, braced initialisers) is one assertion (/2.1 "__VA_ARGS__ is
// evaluated and contextually converted to bool"); /1 with NDEBUG defined where <cassert> is
// included it is ((void)0) and the argument is not evaluated; /4 it "is redefined according to
// the current state of NDEBUG each time that <cassert> is included"; /5 assert(E) is a
// constant subexpression when E is a constant true (or NDEBUG); /2.3 a failing assertion writes
// a diagnostic containing #__VA_ARGS__, the file name, the line and the function name to the
// standard error stream and calls abort() (checked in a child process).
#include <cassert>
#include <type_traits>
#include <utility>
#include "child_process.hpp"
#include "check.hpp"

template <class A, class B>
constexpr bool same() {
  return std::is_same_v<A, B>;
}

constexpr int checked_half(int x) {
  assert(x % 2 == 0 && same<int, int>());
  return x / 2;
}
static_assert(checked_half(8) == 4);  // assert(E) with E true is a constant subexpression

static int evaluations = 0;
static bool count_true() {
  ++evaluations;
  return true;
}

static constexpr int fail_line = __LINE__ + 2;
static void failing_function_name_xyz() {
  assert(same<int, long>() && "commas, inside");
}

static void with_ndebug();
static int after_reinclude();

int main(int, char**) {
  if (child_mode_is("fail")) {
    failing_function_name_xyz();
    return 0;  // not reached
  }
  assert(same<int, int>());
  assert(std::pair<int, int>{1, 2}.second == 2);
  assert(count_true());
  CHECK(evaluations == 1);
  static_assert(std::is_void_v<decltype(assert(true))>);
  with_ndebug();

  ChildResult r = run_self("fail");
  CHECK(r.status == 1000 + SIGABRT);
  CHECK(r.err.find("same<int, long>() && \"commas, inside\"") != std::string::npos);
  CHECK(r.err.find("assert_variadic_ndebug.pass.cpp") != std::string::npos);
  CHECK(r.err.find("failing_function_name_xyz") != std::string::npos);
  CHECK(r.err.find(std::to_string(fail_line)) != std::string::npos);
  CHECK(after_reinclude() == 2);
  return 0;
}

#define NDEBUG
#include <cassert>

static void with_ndebug() {
  assert(count_true());  // not evaluated
  assert(false, "extra", "arguments");  // ((void)0): the arguments are discarded
  static_assert(std::is_void_v<decltype(assert(false))>);
  CHECK(evaluations == 1);
}

#undef NDEBUG
#include <cassert>

static int after_reinclude() {
  assert(count_true());  // active again
  return evaluations;
}
