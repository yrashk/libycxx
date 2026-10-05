// [stacktrace.format]/1-2: formatter<stacktrace_entry> accepts "fill-and-align_opt width_opt"
// and formats as if by copying to_string(se) "with additional padding and adjustments as
// specified by the format specifiers" (left-aligned by default: [format.string.std] Table
// 104, a non-arithmetic non-pointer type); /3-4: for formatter<basic_stacktrace<Allocator>>
// "format-spec is empty", and the stacktrace formats as to_string(s). Any other format-spec
// is not a format string for the argument, which vformat reports as format_error
// ([format.err.report]/1). [stacktrace.entry.obs]/1: a default-constructed entry is empty.
// REQUIRES: exceptions
#include <stacktrace>
#include <format>
#include <algorithm>
#include <string>
#include <string_view>
#include "check.hpp"

template <class T>
bool throws(std::string_view fmt, const T& v) {
  try {
    (void)std::vformat(fmt, std::make_format_args(v));
  } catch (const std::format_error&) {
    return true;
  }
  return false;
}

int main() {
  const std::stacktrace st = std::stacktrace::current();
  CHECK(!st.empty());
  const std::stacktrace_entry e = st[0];
  const std::string s = std::to_string(e);
  CHECK(std::format("{}", e) == s);
  const int w = static_cast<int>(s.size()) + 6;
  CHECK(std::vformat("{:{}}", std::make_format_args(e, w)) == s + std::string(6, ' '));
  CHECK(std::vformat("{:*>{}}", std::make_format_args(e, w)) == std::string(6, '*') + s);
  CHECK(std::vformat("{:-^{}}", std::make_format_args(e, w)) == "---" + s + "---");
  CHECK(std::format("{:1}", e) == s || s.empty());
  const std::stacktrace_entry empty;
  CHECK(std::format("{}", empty) == std::to_string(empty));
  CHECK(std::format("[{:>3}]", empty) == "[" + std::string(3 - std::min<std::size_t>(3, std::to_string(empty).size()), ' ') + std::to_string(empty) + "]");
  CHECK(throws("{:.3}", e) && throws("{:s}", e) && throws("{:+}", e) && throws("{:05}", e));
  // basic_stacktrace: only an empty format-spec.
  CHECK(std::format("{}", st) == std::to_string(st));
  CHECK(std::format("{}", std::stacktrace()) == std::to_string(std::stacktrace()));
  CHECK(throws("{:10}", st) && throws("{:<}", st) && throws("{:s}", st));
  CHECK(!throws("{:}", st));
  return 0;
}
