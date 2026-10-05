// [exception]/2: "Except where explicitly specified otherwise, each standard library class T
// that derives from class exception has the following publicly accessible member functions,
// each of them having a non-throwing exception specification: default constructor (unless the
// class synopsis shows other constructors), copy constructor, copy assignment operator. The copy
// constructor and the copy assignment operator meet the following postcondition: If two objects
// lhs and rhs both have dynamic type T and lhs is a copy of rhs, then strcmp(lhs.what(),
// rhs.what()) is equal to 0."
// Checked statically for every such class, and at run time while operator new fails at its k-th
// call (a copy that allocated, for instance the message or filesystem_error's paths, would have
// to throw, or terminate): no copy or copy assignment may throw or allocate a block it leaks,
// and the copies have equal what() strings. [syserr.syserr.members], [fs.filesystem.error.members]
// path1()/path2() and [time.zone.exception] are checked on the copies as well.
#include <any>
#include <chrono>
#include <cstring>
#include <exception>
#include <expected>
#include <filesystem>
#include <format>
#include <functional>
#include <future>
#include <ios>
#include <memory>
#include <new>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <typeinfo>
#include <variant>
#include "exc_new.hpp"

using namespace exh;
namespace fs = std::filesystem;

template <class T>
constexpr bool nothrow_copy = std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>;

static_assert(nothrow_copy<std::exception> && nothrow_copy<std::logic_error> && nothrow_copy<std::domain_error> &&
              nothrow_copy<std::invalid_argument> && nothrow_copy<std::length_error> &&
              nothrow_copy<std::out_of_range> && nothrow_copy<std::runtime_error> && nothrow_copy<std::range_error> &&
              nothrow_copy<std::overflow_error> && nothrow_copy<std::underflow_error>);
static_assert(nothrow_copy<std::system_error> && nothrow_copy<std::ios_base::failure> &&
              nothrow_copy<fs::filesystem_error> && nothrow_copy<std::future_error> && nothrow_copy<std::regex_error> &&
              nothrow_copy<std::format_error>);
static_assert(nothrow_copy<std::bad_alloc> && nothrow_copy<std::bad_array_new_length> && nothrow_copy<std::bad_cast> &&
              nothrow_copy<std::bad_typeid> && nothrow_copy<std::bad_exception> && nothrow_copy<std::bad_weak_ptr> &&
              nothrow_copy<std::bad_function_call> && nothrow_copy<std::bad_optional_access> &&
              nothrow_copy<std::bad_variant_access> && nothrow_copy<std::bad_any_cast> &&
              nothrow_copy<std::bad_expected_access<int>> && nothrow_copy<std::chrono::nonexistent_local_time> &&
              nothrow_copy<std::chrono::ambiguous_local_time>);
static_assert(std::is_nothrow_default_constructible_v<std::bad_alloc> &&
              std::is_nothrow_default_constructible_v<std::bad_array_new_length> &&
              std::is_nothrow_default_constructible_v<std::bad_cast> &&
              std::is_nothrow_default_constructible_v<std::bad_typeid> &&
              std::is_nothrow_default_constructible_v<std::bad_exception> &&
              std::is_nothrow_default_constructible_v<std::bad_weak_ptr> &&
              std::is_nothrow_default_constructible_v<std::bad_function_call> &&
              std::is_nothrow_default_constructible_v<std::bad_optional_access> &&
              std::is_nothrow_default_constructible_v<std::bad_variant_access> &&
              std::is_nothrow_default_constructible_v<std::bad_any_cast>);

template <class T>
void copies(const char* name, const T& src, const T& other) {
  sweep_new(name, [&] {
    bool threw = attempt([&] {
      T a(src);
      T b(other);
      b = a;
      const T& ca = a;
      T c(ca);
      c = b;
      EXH_EXPECT(std::strcmp(a.what(), src.what()) == 0 && std::strcmp(b.what(), src.what()) == 0 &&
                     std::strcmp(c.what(), src.what()) == 0,
                 "a copy has another what()");
      if constexpr (std::is_base_of_v<std::system_error, T>) EXH_EXPECT(c.code() == src.code(), "code()");
      if constexpr (std::is_same_v<T, fs::filesystem_error>)
        EXH_EXPECT(c.path1() == src.path1() && c.path2() == src.path2(), "path1/path2");
      if constexpr (std::is_same_v<T, std::regex_error>) EXH_EXPECT(c.code() == src.code(), "regex_error::code");
    });
    EXH_EXPECT(!threw, "copying an exception object threw");
    return st.fired;
  }, options{true, 4000});
}

int main() {
  const std::string long_msg(200, 'm');
  copies("logic_error", std::logic_error(long_msg), std::logic_error("other"));
  copies("domain_error", std::domain_error(long_msg), std::domain_error("other"));
  copies("invalid_argument", std::invalid_argument(long_msg), std::invalid_argument("other"));
  copies("length_error", std::length_error(long_msg), std::length_error("other"));
  copies("out_of_range", std::out_of_range(long_msg), std::out_of_range("other"));
  copies("runtime_error", std::runtime_error(long_msg), std::runtime_error("other"));
  copies("range_error", std::range_error(long_msg), std::range_error("other"));
  copies("overflow_error", std::overflow_error(long_msg), std::overflow_error("other"));
  copies("underflow_error", std::underflow_error(long_msg), std::underflow_error("other"));
  copies("system_error", std::system_error(std::make_error_code(std::errc::no_such_device), long_msg),
         std::system_error(std::make_error_code(std::errc::io_error)));
  copies("ios_base::failure", std::ios_base::failure(long_msg), std::ios_base::failure("other"));
  copies("filesystem_error",
         fs::filesystem_error(long_msg, fs::path("/first/long/path/" + long_msg), fs::path("second/" + long_msg),
                              std::make_error_code(std::errc::file_exists)),
         fs::filesystem_error("other", std::make_error_code(std::errc::io_error)));
  copies("future_error", std::future_error(std::future_errc::broken_promise),
         std::future_error(std::future_errc::no_state));
  copies("regex_error", std::regex_error(std::regex_constants::error_brack), std::regex_error(std::regex_constants::error_paren));
  copies("format_error", std::format_error(long_msg), std::format_error("other"));
  copies("bad_alloc", std::bad_alloc(), std::bad_alloc());
  copies("bad_array_new_length", std::bad_array_new_length(), std::bad_array_new_length());
  copies("bad_cast", std::bad_cast(), std::bad_cast());
  copies("bad_typeid", std::bad_typeid(), std::bad_typeid());
  copies("bad_exception", std::bad_exception(), std::bad_exception());
  copies("bad_weak_ptr", std::bad_weak_ptr(), std::bad_weak_ptr());
  copies("bad_function_call", std::bad_function_call(), std::bad_function_call());
  copies("bad_optional_access", std::bad_optional_access(), std::bad_optional_access());
  copies("bad_variant_access", std::bad_variant_access(), std::bad_variant_access());
  copies("bad_any_cast", std::bad_any_cast(), std::bad_any_cast());
  copies("bad_expected_access<int>", std::bad_expected_access<int>(5), std::bad_expected_access<int>(6));

  using namespace std::chrono;
  const time_zone* ny = locate_zone("America/New_York");
  local_seconds gap = local_days(2021y / March / 14) + 2h + 30min;      // skipped (DST starts)
  local_seconds twice = local_days(2021y / November / 7) + 1h + 30min;  // repeated (DST ends)
  local_info gi = ny->get_info(gap), ti = ny->get_info(twice);
  if (gi.result == local_info::nonexistent && ti.result == local_info::ambiguous) {
    copies("nonexistent_local_time", nonexistent_local_time(gap, gi), nonexistent_local_time(gap, gi));
    copies("ambiguous_local_time", ambiguous_local_time(twice, ti), ambiguous_local_time(twice, ti));
  }
  return finish();
}
