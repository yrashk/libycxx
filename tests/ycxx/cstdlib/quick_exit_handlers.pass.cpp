// quick_exit and at_quick_exit, run in child processes ([support.start.term]).
//   /10: at_quick_exit registers f "to be called without arguments when quick_exit is called";
//     Note 5: "The at_quick_exit registrations are distinct from the atexit registrations".
//   /11: "The implementation shall support the registration of at least 32 functions."
//   /12: "Returns: Zero if the registration succeeds".
//   /13: "Functions registered by calls to at_quick_exit are called in the reverse order of
//     their registration" (a function registered twice is called twice, at both positions).
//     "Objects shall not be destroyed as a result of calling quick_exit."
//     "If a registered function invoked by quick_exit exits via an exception, the function
//     std::terminate is invoked." "After calling registered functions, quick_exit shall call
//     _Exit(status)": no atexit functions ([support.start.term]/3) and the status is reported.
//     Note 6: the registered functions run on the thread that calls quick_exit, which can be a
//     thread other than main.
//   Both handler overloads: c-atexit-handler (extern "C") and atexit-handler.
//   /9: exit calls the atexit functions and destroys objects, and not the at_quick_exit ones;
//     [basic.start.term]/1, /6: thread_local objects of the exiting thread first, then the atexit
//     function registered after static_object's construction, then static_object.
// Handlers write with write(2) or with a flushed std::cout: _Exit does not flush C streams, and
// nothing is printed before quick_exit, so the captured output is exactly the handlers' output.
// The handlers use library facilities (format, string, locale): quick_exit destroys no object of
// the library either, so they are all still usable.
// FLAGS: -pthread
#include <cstdlib>
#include <exception>
#include <format>
#include <iostream>
#include <locale>
#include <string>
#include <thread>
#include <utility>
#include "child_process.hpp"
#include "check.hpp"

static void say(const char* s) { (void)!write(1, s, strlen(s)); }

namespace {
struct Static {
  ~Static() { say("static-dtor\n"); }
} static_object;

void h1() { std::cout << std::format("h1 {:>3}|{}\n", 1, std::string(3, 'x')) << std::flush; }
void h2() { say("h2\n"); }
void h3() {
  std::locale loc;
  std::cout << std::use_facet<std::ctype<char>>(loc).toupper('h') << "3\n" << std::flush;
}
void never() { say("atexit-called\n"); }
template <int N>
void numbered() {
  std::string s = std::to_string(N) + ' ';
  say(s.c_str());
}
template <int... N>
bool register_numbered(std::integer_sequence<int, N...>) {
  return ((std::at_quick_exit(numbered<N>) == 0) && ...);
}
}  // namespace

extern "C" void c_handler() { say("c\n"); }

[[noreturn]] static void on_terminate() {
  say("terminate\n");
  std::_Exit(9);
}

static int child(const std::string& mode) {
  thread_local Static tl;  // not destroyed by quick_exit either
  (void)&tl;
  if (std::atexit(never) != 0) return 101;
  if (mode == "order") {
    if (std::at_quick_exit(h1) || std::at_quick_exit(h2) || std::at_quick_exit(h3) || std::at_quick_exit(h2) ||
        std::at_quick_exit(c_handler))
      return 102;
    std::quick_exit(42);
  }
  if (mode == "limit") {
    if (!register_numbered(std::make_integer_sequence<int, 32>())) return 103;
    std::quick_exit(0);
  }
  if (mode == "thread") {
    if (std::at_quick_exit(h2) || std::at_quick_exit(h1)) return 104;
    std::thread t([] { std::quick_exit(43); });
    int p[2];
    if (pipe(p) != 0) return 105;
    char c;
    (void)!read(p[0], &c, 1);  // blocks until the process ends
    t.join();
    return 106;
  }
  if (mode == "throw") {
    std::set_terminate(on_terminate);
    if (std::at_quick_exit(h2) || std::at_quick_exit([] { throw 1; })) return 107;
    std::quick_exit(44);
  }
  if (mode == "exit") {
    if (std::at_quick_exit(h2)) return 108;
    std::exit(45);
  }
  return 110;
}

int main(int argc, char** argv) {
  if (child_mode()) return child(argc > 1 ? argv[1] : "");
  ChildResult r = run_self("order");
  CHECK(same_text(r.out, "c\nh2\nH3\nh2\nh1   1|xxx\n", "order stdout"));
  CHECK(r.status == 42 && r.err.empty());

  r = run_self("limit");
  std::string want;
  for (int i = 31; i >= 0; --i) want += std::to_string(i) + ' ';
  CHECK(same_text(r.out, want, "limit stdout"));
  CHECK(r.status == 0 && r.err.empty());

  r = run_self("thread");
  CHECK(same_text(r.out, "h1   1|xxx\nh2\n", "thread stdout"));
  CHECK(r.status == 43 && r.err.empty());

  r = run_self("throw");
  CHECK(same_text(r.out, "terminate\n", "throw stdout"));
  CHECK(r.status == 9);

  r = run_self("exit");
  CHECK(same_text(r.out, "static-dtor\natexit-called\nstatic-dtor\n", "exit stdout"));
  CHECK(r.status == 45 && r.err.empty());
  return 0;
}
