// [csignal.syn]: std::signal, std::raise, std::sig_atomic_t, SIG_DFL/SIG_ERR/SIG_IGN and the
// signal numbers (ISO C 7.14: signal returns the previous handler; raise does not return until
// the handler has returned). [support.signal]/1: "A call to the function signal synchronizes
// with any resulting invocation of the signal handler so installed"; /2-3: a handler may use
// plain lock-free atomic operations and the functions explicitly identified as signal-safe:
// [forward] (forward, forward_like, move, move_if_noexcept: "All functions specified in this
// subclause are signal-safe"), [support.initlist] (initializer_list's members and begin/end),
// [cstring.syn] memcpy and memmove, [support.start.term] _Exit, abort and quick_exit (when
// the functions registered with at_quick_exit are signal-safe), /4 signal itself for the
// signal being handled; [atomics.fences] atomic_signal_fence is a plain atomic function.
// Handlers here use only those; the exit paths run in child processes.
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include "child_process.hpp"
#include "check.hpp"

static_assert(std::is_integral_v<std::sig_atomic_t>);
static_assert(std::atomic<int>::is_always_lock_free);

static volatile std::sig_atomic_t hits = 0;
static std::atomic<int> atomic_hits{0};
static std::atomic_flag flag;
static int plain_before = 0;   // written before signal(): visible in the handler ([support.signal]/1)
static int seen_in_handler = 0;
static char copy_buf[16];
static int forwarded = 0;

struct Moveable {
  int v;
};

extern "C" void handler(int sig) {
  hits = hits + 1;
  atomic_hits.fetch_add(1, std::memory_order_relaxed);
  (void)flag.test_and_set();
  std::atomic_signal_fence(std::memory_order_seq_cst);
  seen_in_handler = plain_before;
  const char src[] = "handler!";
  std::memcpy(copy_buf, src, sizeof src);
  std::memmove(copy_buf + 1, copy_buf, 4);  // overlapping
  std::initializer_list<int> il = {1, 2, 3};
  int s = 0;
  for (int x : il) s += x;
  Moveable m{static_cast<int>(il.size())};
  Moveable m2 = std::move(m);
  Moveable m3 = std::move_if_noexcept(m2);
  forwarded = s + std::forward<Moveable&>(m3).v + std::forward_like<int&>(m3.v);
  (void)std::signal(sig, handler);  // [support.signal]/4: same signal number
}

extern "C" void exit_handler(int) { std::_Exit(42); }
extern "C" void abort_handler(int) { std::abort(); }
extern "C" void at_quick() {
  const char msg[] = "quick";
  (void)write(1, msg, 5);  // POSIX async-signal-safe; not a standard library function
}
extern "C" void quick_handler(int) { std::quick_exit(43); }

int main(int argc, char** argv) {
  (void)argc;
  if (child_mode()) {
    std::string m = argv[1];
    std::signal(SIGTERM, SIG_DFL);
    if (m == "exit") std::signal(SIGTERM, exit_handler);
    else if (m == "abort") std::signal(SIGTERM, abort_handler);
    else {
      std::at_quick_exit(at_quick);
      std::signal(SIGTERM, quick_handler);
    }
    std::raise(SIGTERM);
    return 7;  // not reached
  }
  plain_before = 1234;
  std::signal(SIGINT, SIG_DFL);  // the disposition inherited from the parent may be SIG_IGN
  auto prev = std::signal(SIGINT, handler);
  CHECK(prev == SIG_DFL);
  CHECK(std::raise(SIGINT) == 0);
  CHECK(hits == 1 && atomic_hits.load() == 1 && flag.test());
  CHECK(seen_in_handler == 1234);
  CHECK(std::strcmp(copy_buf, "hhander!") == 0);
  CHECK(forwarded == 6 + 3 + 3);
  CHECK(std::raise(SIGINT) == 0);  // the handler reinstalled itself
  CHECK(hits == 2);
  CHECK(std::signal(SIGINT, SIG_IGN) == handler);
  CHECK(std::raise(SIGINT) == 0);  // ignored
  CHECK(hits == 2);
  CHECK(std::signal(SIGINT, SIG_DFL) == SIG_IGN);
  CHECK(std::signal(-1, handler) == SIG_ERR);

  ChildResult e = run_self("exit");
  CHECK(e.status == 42);
  ChildResult a = run_self("abort");
  CHECK(a.status == 1000 + SIGABRT);
  ChildResult q = run_self("quick");
  CHECK(q.status == 43);
  CHECK(q.out == "quick");
}
