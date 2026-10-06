// libycxx hosted runtime: std::contracts::invoke_default_contract_violation_handler
// ([support.contract.invoke]): the default contract-violation handler reports the violation on
// stderr (through the PAL, with no allocation) and returns; the caller then continues
// (observe) or terminates (enforce).
#include <contracts>
#include <ycxx/pal.h>

#include <charconv>
#include <cstring>

namespace {

void put(const char* s) noexcept {
  ycxx_pal_size n = std::strlen(s), done = 0;
  while (done < n) {
    ycxx_pal_size __w = 0;
    if (ycxx_pal_write(ycxx_pal_stderr, s + done, n - done, &__w) != 0 || __w == 0)
      return;
    done += __w;
  }
}

const char* kind_name(std::contracts::assertion_kind k) noexcept {
  switch (k) {
  case std::contracts::assertion_kind::pre: return "pre";
  case std::contracts::assertion_kind::post: return "post";
  case std::contracts::assertion_kind::assert: return "assert";
  }
  return "unknown";
}
const char* semantic_name(std::contracts::evaluation_semantic s) noexcept {
  switch (s) {
  case std::contracts::evaluation_semantic::ignore: return "ignore";
  case std::contracts::evaluation_semantic::observe: return "observe";
  case std::contracts::evaluation_semantic::enforce: return "enforce";
  case std::contracts::evaluation_semantic::quick_enforce: return "quick_enforce";
  }
  return "unknown";
}
const char* mode_name(std::contracts::detection_mode m) noexcept {
  switch (m) {
  case std::contracts::detection_mode::predicate_false: return "predicate_false";
  case std::contracts::detection_mode::evaluation_exception: return "evaluation_exception";
  }
  return "unknown";
}

} // namespace

// "contract violation in function F at FILE:LINE: COMMENT" and the classification.
void std::contracts::invoke_default_contract_violation_handler(const contract_violation& __v) {
  const source_location __loc = __v.location();
  put("contract violation in function ");
  put(*__loc.function_name() ? __loc.function_name() : "<unknown>");
  put(" at ");
  put(*__loc.file_name() ? __loc.file_name() : "<unknown>");
  char line[16] = {};
  *std::to_chars(line, line + sizeof line - 1, __loc.line()).ptr = '\0';
  put(":");
  put(line);
  put(": ");
  put(__v.comment());
  put("\n[assertion_kind: ");
  put(kind_name(__v.kind()));
  put(", semantic: ");
  put(semantic_name(__v.semantic()));
  put(", mode: ");
  put(mode_name(__v.detection_mode()));
  put(__v.is_terminating() ? ", terminating: yes]\n" : ", terminating: no]\n");
}
