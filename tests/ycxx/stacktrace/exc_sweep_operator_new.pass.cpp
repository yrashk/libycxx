// basic_stacktrace<allocator<stacktrace_entry>> while operator new fails at its k-th call, for
// every k.
//   [stacktrace.basic.cons]/1, /3, /6: current(), current(skip), current(skip, max_depth) are
//     noexcept and return "an empty basic_stacktrace object if the initialization of frames_
//     failed": an allocation failure (std::allocator obtains storage from operator new,
//     [allocator.members]/5) must yield an empty stacktrace, never std::terminate.
//   [stacktrace.entry.query], [stacktrace.basic.nonmem]: description(), source_file(),
//     to_string() may throw bad_alloc (or handle an allocation failure by giving less
//     information); nothing leaks.
// After every run every operator new block is freed (see the note on the last sweep).
// REQUIRES: exceptions
#include <stacktrace>
#include <string>
#include "exc_new.hpp"

using namespace exh;

[[gnu::noinline]] static std::stacktrace capture(int depth) {
  if (depth == 0) return std::stacktrace::current();
  std::stacktrace s = capture(depth - 1);
  return s;
}

int main() {
  // Warm-up with nothing armed: symbolization may fill caches that live until exit.
  for (int i = 0; i < 2; ++i) {
    std::stacktrace s = capture(1);
    std::string a = std::to_string(s);
    if (!s.empty()) a = s[0].description() + s[0].source_file();
  }
  sweep_new("stacktrace::current() (noexcept: empty on failure)", [] {
    std::size_t n = 0;
    bool threw = attempt([&] { n = capture(5).size(); });
    EXH_EXPECT(!threw, "current() is noexcept; nothing may propagate");
    return threw;
  }, options{.may_swallow = true});
  sweep_new("stacktrace::current(1, 3)", [] {
    bool threw = attempt([] { std::stacktrace s = std::stacktrace::current(1, 3); });
    EXH_EXPECT(!threw, "current(skip, max_depth) is noexcept");
    return threw;
  }, options{.may_swallow = true});
  // Symbolization may create internal state (caches, scratch buffers) on paths first reached
  // after an allocation failure and keep it until exit: that is not a leak. The sweep is run
  // once without the operator new balance check, then again with it: the second, identical
  // sweep must not lose any block.
  for (int pass = 0; pass < 2; ++pass) {
    auto scenario = [] {
      std::stacktrace s = capture(1);
      return attempt([&] {
        std::stacktrace c(s);
        if (!c.empty()) {
          std::string a = std::to_string(c[0]);
          std::string d = c[0].description();
          std::string f = c[0].source_file();
        }
      });
    };
    if (pass == 0)
      sweep("stacktrace copy, to_string, description (first pass)", gnew, scenario, options{.may_swallow = true});
    else
      sweep_new("stacktrace copy, to_string, description", scenario, options{.may_swallow = true});
  }
  return finish();
}
