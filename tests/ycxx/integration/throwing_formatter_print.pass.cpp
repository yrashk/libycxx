// An exception thrown by a program-defined formatter, propagated through std::format,
// std::print and std::println.
//   [formatter.requirements]: format() of a program-defined formatter may throw;
//   [format.err.report]/1: formatting functions "propagate exceptions thrown by operations of
//   formatter specializations and iterators".
//   [print.fun]/2: print(FILE*, ...) for an argument type with
//   enable_nonlocking_formatter_optimization false (the default, [format.formatter.locking]) is
//   vprint_unicode_buffered / vprint_nonunicode_buffered, i.e. (/8, /14) "string out =
//   vformat(fmt, args);" first: when vformat throws, NOTHING is written. /11, /17: "Throws: Any
//   exception thrown by the call to vformat". /10: the unbuffered vprint_unicode
//   "Unconditionally unlocks stream on function exit", so with a locksafe type (the trait
//   specialized to true) output may be partial, but another thread can still print to the same
//   FILE afterwards. println(FILE*, ...) (/5) is print with "{}\n" appended.
//   [ostream.formatted.print]/4: vprint_*(ostream&): "any exception thrown by the call to vformat
//   is propagated without regard to the value of os.exceptions() and without turning on
//   ios_base::badbit"; the output is the string out, so nothing is inserted. println(os, ...)
//   (/2) formats first. [ostream.sentry]/4: ~sentry() calls pubsync for unitbuf only if
//   !uncaught_exceptions(): no sync while the formatter's exception propagates.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <cstdio>
#include <format>
#include <ostream>
#include <print>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>
#include "watchdog.hpp"
#include "check.hpp"

struct Boom {
  int code;
};
struct Bad {};
struct BadLocksafe {};

template <class T>
  requires std::same_as<T, Bad> || std::same_as<T, BadLocksafe>
struct std::formatter<T> {
  constexpr auto parse(std::format_parse_context& pc) { return pc.begin(); }
  template <class FC>
  typename FC::iterator format(const T&, FC& fc) const {
    auto out = fc.out();
    *out++ = '<';  // partial output into the context before the throw
    throw Boom{7};
  }
};
template <>
constexpr bool std::enable_nonlocking_formatter_optimization<BadLocksafe> = true;

static std::string contents(std::FILE* f) {
  CHECK(std::fflush(f) == 0);
  std::rewind(f);
  std::string s;
  int c;
  while ((c = std::fgetc(f)) != EOF) s += static_cast<char>(c);
  std::fseek(f, 0, SEEK_END);
  return s;
}

template <class F>
static bool throws_boom(F f) {
  try {
    f();
  } catch (const Boom& b) {
    return b.code == 7;
  }
  return false;
}

struct SyncCounter : std::stringbuf {
  int syncs = 0;
  int sync() override {
    ++syncs;
    return 0;
  }
};

int main() {
  watchdog(30);
  CHECK(throws_boom([] { (void)std::format("a{}b", Bad{}); }));
  CHECK(throws_boom([] { (void)std::format("a{}b", BadLocksafe{}); }));
  {
    std::string s = "pre:";
    CHECK(throws_boom([&] { std::format_to(std::back_inserter(s), "a{}b", Bad{}); }));
    CHECK(s.starts_with("pre:"));  // partial output into s is allowed
  }

  // print / println to a FILE*, buffered path: nothing written.
  std::FILE* f = std::tmpfile();
  CHECK(f != nullptr);
  CHECK(throws_boom([&] { std::print(f, "abc {} def {}", 1, Bad{}); }));
  CHECK(throws_boom([&] { std::println(f, "abc {} def {}", 1, Bad{}); }));
  CHECK(contents(f).empty());
  std::print(f, "ok{}", 1);
  CHECK(contents(f) == "ok1");
  // Unbuffered path (locksafe argument): partial output allowed, the stream is unlocked after.
  CHECK(throws_boom([&] { std::print(f, "[{}]", BadLocksafe{}); }));
  CHECK(throws_boom([&] { std::println(f, "[{}]", BadLocksafe{}); }));
  {
    std::string s = contents(f);
    CHECK(s.starts_with("ok1"));
    CHECK(s.find(']') == std::string::npos);  // nothing after the failed argument
  }
  std::thread([f] { std::print(f, "|T{}|", 2); }).join();  // would deadlock if f were left locked
  CHECK(contents(f).ends_with("|T2|"));
  std::fclose(f);

  // print / println to an ostream.
  for (bool exceptions : {false, true}) {
    SyncCounter buf;
    std::ostream os(&buf);
    if (exceptions) os.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    os.setf(std::ios_base::unitbuf);
    CHECK(throws_boom([&] { std::print(os, "x{}y", Bad{}); }));
    CHECK(os.rdstate() == std::ios_base::goodbit);
    CHECK(buf.str().empty());
    CHECK(buf.syncs == 0);  // ~sentry with uncaught_exceptions() != 0: no pubsync
    CHECK(throws_boom([&] { std::println(os, "x{}y", Bad{}); }));
    CHECK(throws_boom([&] { std::print(os, "x{}y", BadLocksafe{}); }));
    CHECK(os.rdstate() == std::ios_base::goodbit);
    CHECK(buf.str().empty());
    CHECK(buf.syncs == 0);
    std::print(os, "fine{}", 3);
    CHECK(buf.str() == "fine3");
    CHECK(buf.syncs >= 1);  // unitbuf, no exception: pubsync
    std::println(os, "{}", "!");
    CHECK(buf.str() == "fine3!\n");
  }
  return 0;
}
