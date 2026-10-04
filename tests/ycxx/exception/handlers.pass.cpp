// [exception.syn], [set.terminate], [get.terminate]: terminate_handler = void (*)();
// set_terminate returns the previous handler; get_terminate returns the current one; both
// noexcept. [exception]: class exception has noexcept special members and virtual what().
// [bad.exception]: bad_exception derives from exception.
#include <exception>
#include <type_traits>
#include "check.hpp"

[[noreturn]] void my_terminate() { __builtin_abort(); }

static_assert(std::is_same_v<std::terminate_handler, void (*)()>);
static_assert(noexcept(std::get_terminate()));
static_assert(noexcept(std::set_terminate(nullptr)));
static_assert(noexcept(std::terminate()));
static_assert(std::is_nothrow_default_constructible_v<std::exception>);
static_assert(std::is_nothrow_copy_constructible_v<std::exception>);
static_assert(std::is_nothrow_copy_assignable_v<std::exception>);
static_assert(std::has_virtual_destructor_v<std::exception>);
static_assert(noexcept(std::declval<const std::exception&>().what()));
static_assert(std::is_base_of_v<std::exception, std::bad_exception>);
static_assert(std::is_nothrow_default_constructible_v<std::bad_exception>);

int main() {
  std::terminate_handler old = std::set_terminate(my_terminate);
  CHECK(std::get_terminate() == my_terminate);
  CHECK(std::set_terminate(old) == my_terminate);
  CHECK(std::get_terminate() == old);
  std::exception e;
  CHECK(e.what() != nullptr);
  std::bad_exception be;
  CHECK(be.what() != nullptr);
  return 0;
}
