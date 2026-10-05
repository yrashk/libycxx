// A thread manager with static storage duration ([basic.start.term]/7 Note: "These requirements
// permit thread managers as static-storage-duration objects"): a static jthread, constructed
// before main, whose thread keeps using the library until the jthread's destructor requests
// stop and joins it. Its uses all happen before that destructor completes, so before the
// completion of the destruction of objects with static storage duration: they are not among the
// uses [basic.start.term]/7 makes undefined, and must keep working while the other static
// objects are destroyed -- including any the library created lazily during main (constructed
// after the jthread, so destroyed before it: [basic.start.term]/4).
// The worker waits until main has used every facility (so whatever the library creates lazily
// exists), then loops. A static object constructed between the jthread and main sleeps in its
// destructor (with nanosleep), keeping the worker running after the library's own objects of
// static storage duration, if any, have been destroyed. The worker reports through write(2);
// the program runs in a child process whose exit status and output are checked (run also with
// SANITIZER=asan, which reports a use of destroyed library state).
//   [thread.jthread.cons]/? ~jthread: "If joinable() is true, calls request_stop() and then
//     join()"; [iostream.objects.overview]/3; [mem.res.global]; [locale.statics];
//     [syserr.errcat.objects]; [time.zone.db.access].
// FLAGS: -pthread
// REQUIRES: exceptions
#include <atomic>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <exception>
#include <format>
#include <iostream>
#include <locale>
#include <memory_resource>
#include <random>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <system_error>
#include <thread>
#include <sched.h>
#include <time.h>
#include <vector>
#include "child_process.hpp"
#include "check.hpp"

static bool battery() {
  bool ok = true;
  ok &= std::format("{:>5}|{:.3f}|{:x}", "ab", 3.14159, 255) == "   ab|3.142|ff";
  std::ostringstream os;
  os << 1.5 << ' ' << std::boolalpha << true;
  ok &= os.str() == "1.5 true";
  ok &= std::use_facet<std::ctype<char>>(std::locale()).toupper('a') == 'A';
  ok &= std::use_facet<std::numpunct<char>>(std::locale::classic()).decimal_point() == '.';
  ok &= std::regex_match("abc123", std::regex("[a-z]+[0-9]+"));
  ok &= !std::generic_category().message(EDOM).empty();
  ok &= std::system_category().default_error_condition(EDOM) == std::errc::argument_out_of_domain;
  try {
    throw std::system_error(std::make_error_code(std::errc::io_error), "ctx");
  } catch (const std::system_error& e) {
    ok &= e.code() == std::errc::io_error;
  }
  std::pmr::vector<std::string> pv(std::pmr::get_default_resource());
  pv.emplace_back("a long string that does not fit in the small buffer");
  void* q = std::pmr::new_delete_resource()->allocate(64, 16);
  std::pmr::new_delete_resource()->deallocate(q, 64, 16);
  std::pmr::synchronized_pool_resource pool;
  std::pmr::vector<int> iv({1, 2, 3}, &pool);
  ok &= iv.size() == 3;
  ok &= std::chrono::locate_zone("UTC") != nullptr;
  char buf[32];
  auto [p, ec] = std::to_chars(buf, buf + sizeof buf, 0.1);
  ok &= ec == std::errc() && std::string(buf, p) == "0.1";
  std::exception_ptr ep = std::make_exception_ptr(std::runtime_error("ep"));
  try {
    std::rethrow_exception(ep);
  } catch (const std::runtime_error&) {
  }
  std::mt19937 g;
  ok &= std::uniform_int_distribution<int>(1, 6)(g) >= 1;
  return ok;
}

static void say(const char* s) { (void)!write(1, s, strlen(s)); }

namespace {
std::atomic<int> phase{0};  // 1: main has used the library; 2: main has returned
std::atomic<long> after_main{0};
std::atomic<bool> failed{false};

struct Manager {
  std::jthread worker;
  Manager() {
    if (!getenv("YCXX_CHILD_MODE")) return;
    worker = std::jthread([](std::stop_token st) {
      while (phase.load() == 0) sched_yield();
      while (!st.stop_requested()) {
        if (!battery()) failed = true;
        if (phase.load() == 2) ++after_main;
      }
      say(failed ? "worker: wrong results\n" : "worker: ok\n");
      if (after_main.load() == 0) say("worker: did not run during termination\n");
    });
  }
} manager;

struct Sleeper {
  ~Sleeper() {
    if (!getenv("YCXX_CHILD_MODE")) return;
    phase = 2;
    timespec ts{0, 200 * 1000 * 1000};
    nanosleep(&ts, nullptr);
  }
} sleeper;
}  // namespace

int main() {
  if (child_mode()) {
    if (!battery()) return 3;
    phase = 1;
    timespec ts{0, 20 * 1000 * 1000};
    nanosleep(&ts, nullptr);
    return 0;
  }
  ChildResult r = run_self("run");
  CHECK(same_text(r.out, "worker: ok\n", "stdout"));
  CHECK(r.status == 0);
  CHECK(r.err.empty());
  return 0;
}
