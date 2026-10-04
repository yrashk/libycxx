// The ways a whole program reaches std::terminate through the library and the termination
// sequence, each run in a child process whose terminate handler ([terminate.handler]) reports
// the currently handled exception ([except.handle]/9: "an implicit handler is considered active
// when the function std::terminate is entered due to a throw"; [propagation]/9
// current_exception) and exits with status 7.
//   thread / jthread: [thread.thread.constr]/6, [thread.jthread.cons]/6: "If the invocation of
//     invoke ... terminates with an uncaught exception, terminate is invoked".
//   atexit function: [support.start.term]/9.1: "If a registered function invoked by exit exits
//     via an exception, the function std::terminate is invoked".
//   static and thread_local destructors: [basic.start.term]/4: "If the destruction of an object
//     with static or thread storage duration exits via an exception, the function
//     std::terminate is called" (the destructors here are noexcept(false)).
//   main: [except.handle]/7: no matching handler -> std::terminate.
//   control: std::async ([futures.async]/4) stores the exception in the shared state; nothing
//     terminates, and get() rethrows it ([futures.unique.future]: get "throws the stored
//     exception").
// FLAGS: -pthread
#include <cstdlib>
#include <exception>
#include <future>
#include <string>
#include <thread>
#include "child_process.hpp"
#include "check.hpp"

[[noreturn]] static void on_terminate() {
  std::string msg = "terminate:none\n";
  if (std::exception_ptr p = std::current_exception()) {
    try {
      std::rethrow_exception(p);
    } catch (int i) {
      msg = "terminate:" + std::to_string(i) + "\n";
    } catch (...) {
      msg = "terminate:other\n";
    }
  }
  (void)!write(2, msg.data(), msg.size());
  std::_Exit(7);
}

namespace {
struct ThrowingDtor {
  int code;
  bool armed = false;
  ~ThrowingDtor() noexcept(false) {
    if (armed) throw code;
  }
};
ThrowingDtor static_obj{40};

void tl_mode() {
  thread_local ThrowingDtor t{50};
  t.armed = true;
}

int child(const std::string& mode) {
  std::set_terminate(on_terminate);
  if (mode == "thread") std::thread([] { throw 10; }).join();
  if (mode == "jthread") std::jthread([] { throw 20; });
  if (mode == "atexit") {
    std::atexit([] { throw 30; });
    return 0;
  }
  if (mode == "static_dtor") {
    static_obj.armed = true;
    return 0;
  }
  if (mode == "thread_local_dtor") std::thread(tl_mode).join();
  if (mode == "main_thread_local_dtor") {
    tl_mode();
    return 0;
  }
  if (mode == "main") throw 60;
  if (mode == "async") {
    auto f = std::async(std::launch::async, []() -> int { throw 70; });
    try {
      f.get();
    } catch (int i) {
      (void)!write(1, "async:", 6);
      std::string s = std::to_string(i) + "\n";
      (void)!write(1, s.data(), s.size());
      return 0;
    }
    return 1;
  }
  return 2;  // not reached for the terminating modes
}
}  // namespace

int main(int argc, char** argv) {
  if (child_mode()) return child(argc > 1 ? argv[1] : "");
  struct {
    const char* mode;
    const char* err;
  } cases[] = {{"thread", "terminate:10\n"},
               {"jthread", "terminate:20\n"},
               {"atexit", "terminate:30\n"},
               {"static_dtor", "terminate:40\n"},
               {"thread_local_dtor", "terminate:50\n"},
               {"main_thread_local_dtor", "terminate:50\n"},
               {"main", "terminate:60\n"}};
  for (auto& c : cases) {
    ChildResult r = run_self(c.mode);
    if (r.status != 7 || r.err != c.err)
      dprintf(2, "mode %s: status %d, stderr [%s]\n", c.mode, r.status, r.err.c_str());
    CHECK(r.status == 7);
    CHECK(r.err == c.err);
  }
  ChildResult r = run_self("async");
  CHECK(r.status == 0 && r.out == "async:70\n" && r.err.empty());
  return 0;
}
