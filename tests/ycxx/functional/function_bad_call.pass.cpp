// [func.wrap.func.inv]/2: operator() "Throws: bad_function_call if !*this; otherwise, any
// exception thrown by the target object." [func.wrap.badcall]: class bad_function_call :
// public exception; what() is noexcept and returns an implementation-defined NTBS.
#include <functional>
#include <exception>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_base_of_v<std::exception, std::bad_function_call>);
static_assert(std::is_convertible_v<std::bad_function_call*, std::exception*>);  // public base
static_assert(std::is_nothrow_default_constructible_v<std::bad_function_call>);
static_assert(std::is_nothrow_copy_constructible_v<std::bad_function_call>);
static_assert(std::is_nothrow_copy_assignable_v<std::bad_function_call>);
static_assert(noexcept(std::declval<const std::bad_function_call&>().what()));

struct Thrower {
  int operator()(int) const { throw 17; }
};

int main() {
  std::function<int(int)> f;
  int stage = 0;
  try {
    f(1);
  } catch (const std::bad_function_call& e) {
    CHECK(e.what() != nullptr);
    stage = 1;
  }
  CHECK(stage == 1);
  try {
    f = nullptr;
    f(1);
  } catch (const std::exception&) {  // catchable as std::exception
    stage = 2;
  }
  CHECK(stage == 2);
  // exceptions from the target propagate unchanged
  f = Thrower{};
  try {
    f(1);
  } catch (int i) {
    CHECK(i == 17);
    stage = 3;
  }
  CHECK(stage == 3);
  std::function<void()> moved_from_empty;
  std::function<void()> dest = std::move(moved_from_empty);
  try {
    dest();
  } catch (const std::bad_function_call&) {
    stage = 4;
  }
  CHECK(stage == 4);
  return 0;
}
