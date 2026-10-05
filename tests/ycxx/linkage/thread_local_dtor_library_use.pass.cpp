// Library use in the destructor of a thread_local object constructed before the thread first
// used the library: any per-thread state the library creates lazily (exception-handling
// globals, caches, buffers) is created after the object, so if it were itself an object with
// thread storage duration it would be destroyed before the object's destructor runs
// ([basic.start.term]/4: reverse order of construction). The destructor's uses happen while
// the thread still runs (before it completes, [basic.start.term]/2), so they are ordinary uses
// and must behave as specified; nothing in the draft ends the library's usability before the
// thread's thread-storage objects are destroyed.
//   Threads: std::thread, std::jthread, std::async(launch::async) ([futures.async]/4.2 "as if
//   in a new thread of execution"), and the main thread (its thread_local objects are destroyed
//   by exit before the static objects, [basic.start.term]/2; checked in a child process).
// The battery: throw and catch (the exception-handling state is per thread: [except.uncaught],
// [propagation]), uncaught_exceptions inside a destructor run during unwinding, format,
// streams and locale facets, regex, error messages, to_chars, pmr, random_device, time zones,
// function, exception_ptr.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <cerrno>
#include <charconv>
#include <chrono>
#include <exception>
#include <format>
#include <functional>
#include <future>
#include <iostream>
#include <locale>
#include <memory_resource>
#include <random>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <vector>
#include "child_process.hpp"
#include "check.hpp"

static int unwinding_seen = -1;
struct DuringUnwind {
  ~DuringUnwind() { unwinding_seen = std::uncaught_exceptions(); }
};

static std::string battery() {
  std::string r;
  auto mark = [&](bool ok) { r += ok ? '.' : 'X'; };
  try {
    DuringUnwind d;
    throw std::runtime_error("tl");
  } catch (const std::exception& e) {
    mark(std::string(e.what()) == "tl" && unwinding_seen == 1 && std::uncaught_exceptions() == 0);
  }
  try {
    try {
      throw 1;
    } catch (int) {
      std::exception_ptr p = std::current_exception();
      std::rethrow_exception(p);
    }
  } catch (int i) {
    mark(i == 1);
  }
  mark(std::format("{:>4}|{:.2f}|{:x}", "ab", 2.5, 255) == "  ab|2.50|ff");
  std::ostringstream os;
  os << std::boolalpha << true << ' ' << 1.25;
  mark(os.str() == "true 1.25");
  std::istringstream is("42 x");
  int n = 0;
  is >> n;
  mark(n == 42);
  mark(std::use_facet<std::ctype<char>>(std::locale()).toupper('z') == 'Z');
  mark(std::regex_search("key=value", std::regex("(\\w+)=(\\w+)")));
  mark(!std::system_category().message(ENOENT).empty() && !std::generic_category().message(EINVAL).empty());
  char buf[32];
  auto [p, ec] = std::to_chars(buf, buf + sizeof buf, 0.3);
  mark(ec == std::errc() && std::string(buf, p) == "0.3");
  std::pmr::unsynchronized_pool_resource pool;
  std::pmr::vector<std::pmr::string> v(&pool);
  v.emplace_back("a pmr string long enough to allocate from the pool resource");
  mark(v.size() == 1);
  std::random_device rd;
  (void)rd();
  mark(true);
  mark(std::chrono::locate_zone("UTC") != nullptr);
  std::function<int()> f = [s = std::string(50, 's')] { return int(s.size()); };
  mark(f() == 50);
  try {
    (void)std::vector<int>().at(1);
  } catch (const std::out_of_range&) {
    mark(true);
  }
  return r;
}

static const std::string want(14, '.');

namespace {
std::string result;
struct LateUser {
  // The constructor uses nothing from the library.
  ~LateUser() { result = battery(); }
};
void in_thread() {
  thread_local LateUser u;
  (void)&u;
  (void)battery();  // creates whatever per-thread state the library uses
}

struct MainUser {
  ~MainUser() { std::cout << "main-thread-local " << battery() << std::endl; }
};
}  // namespace

int main() {
  if (child_mode()) {
    thread_local MainUser u;
    (void)&u;
    std::cout << "main " << battery() << std::endl;
    return 0;
  }
  std::thread(in_thread).join();
  CHECK(same_text(result, want, "thread"));
  result.clear();
  std::jthread(in_thread).join();
  CHECK(same_text(result, want, "jthread"));
  result.clear();
  std::async(std::launch::async, in_thread).get();
  CHECK(same_text(result, want, "async"));

  ChildResult r = run_self("main");
  CHECK(same_text(r.out, "main " + want + "\nmain-thread-local " + want + "\n", "main thread"));
  CHECK(r.status == 0 && r.err.empty());
  return 0;
}
