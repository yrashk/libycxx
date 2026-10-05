// The FIRST call in a process of noexcept library functions that may set up library state
// lazily, while operator new fails: every call from the k-th on fails (memory is exhausted),
// for k = 1, 2, ... until the scenario makes no allocation that fails. Each run is a fresh
// child process (fork), so each run is a first use. An allocation failure that escapes a
// noexcept function calls std::terminate ([except.spec]/5): the child then dies and the run
// fails. Otherwise each function must meet its specification:
//   [locale.cons]/1 locale() noexcept: before any locale::global(), "the resulting facets have
//     virtual function semantics identical to those of locale::classic()" (compared, once
//     memory is available, with classic());
//   [syserr.errcat.objects]/1-4 generic_category(), system_category() noexcept: "All calls to
//     this function shall return references to the same object"; name() is "generic" /
//     "system"; system_category().default_error_condition(0) is error_condition(0,
//     generic_category()); [iostreams.base]/? iostream_category() noexcept, name "iostream";
//     [futures.errors] future_category() noexcept, name "future"; [syserr.errcode.constructors]
//     error_code() noexcept: value 0, category system_category(); make_error_code(errc)
//     noexcept; [syserr.compare] operator== noexcept;
//   [mem.res.global]/1-5 new_delete_resource(), null_memory_resource(), get_default_resource(),
//     set_default_resource() noexcept: "The same value is returned every time"; the default
//     resource is initially new_delete_resource(); set_default_resource returns the previous
//     value;
//   [propagation]/9 current_exception() noexcept: in a handler, an exception_ptr to the handled
//     exception (or a copy), or "If the function needs to allocate memory and the attempt
//     fails, ... an instance of bad_alloc"; null outside handlers; [uncaught.exceptions]
//     uncaught_exceptions() noexcept; [get.terminate] get_terminate() noexcept (the default
//     handler, non-null: [terminate.handler]/?); all of them the first time in a new thread;
//   [rand.device]/? entropy() noexcept: a value in [0, numeric_limits<result_type>::digits];
//   [time.clock.system.members], [time.clock.steady], [time.clock.file.overview]/1 ("noexcept(
//     file_clock::now()) is true"), [time.clock.hires] now() noexcept;
//   [debugging.utility]/2-3 is_debugger_present(), breakpoint_if_debugging() noexcept;
//   [thread.thread.static] hardware_concurrency() noexcept; [thread.thread.this]/1 get_id()
//   noexcept, not equal to thread::id() ([thread.thread.id]/1).
// FLAGS: -pthread
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <debugging>
#include <exception>
#include <future>
#include <ios>
#include <limits>
#include <locale>
#include <memory_resource>
#include <new>
#include <random>
#include <system_error>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"

// operator new: every call from the fail_from-th on fails (0: never).
static long fail_from = 0, calls = 0;
static bool failed = false;
static void* allocate(std::size_t n, std::size_t align, bool nothrow) {
  if (fail_from && __atomic_add_fetch(&calls, 1, __ATOMIC_RELAXED) >= fail_from) {
    __atomic_store_n(&failed, true, __ATOMIC_RELAXED);
    if (nothrow) return nullptr;
    throw std::bad_alloc();
  }
  if (n == 0) n = 1;
  void* p = align <= alignof(std::max_align_t) ? std::malloc(n) : std::aligned_alloc(align, (n + align - 1) / align * align);
  if (!p && !nothrow) throw std::bad_alloc();
  return p;
}
void* operator new(std::size_t n) { return allocate(n, 0, false); }
void* operator new[](std::size_t n) { return allocate(n, 0, false); }
void* operator new(std::size_t n, std::align_val_t a) { return allocate(n, std::size_t(a), false); }
void* operator new[](std::size_t n, std::align_val_t a) { return allocate(n, std::size_t(a), false); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept { return allocate(n, 0, true); }
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept { return allocate(n, 0, true); }
void* operator new(std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept { return allocate(n, std::size_t(a), true); }
void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept { return allocate(n, std::size_t(a), true); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }

static void arm(long k) {
  calls = 0;
  failed = false;
  fail_from = k;
}
static void disarm() { fail_from = 0; }

// Child exit codes: 0 the checks passed and an allocation failed; 10 they passed and nothing
// failed (the sweep is complete); 1 a check failed (reported on stderr).
#define EXPECT(cond)                                                              \
  do {                                                                            \
    if (!(cond)) {                                                                \
      dprintf(2, "k=%ld line %d: %s\n", k, __LINE__, #cond);                      \
      _exit(1);                                                                   \
    }                                                                             \
  } while (0)

static int scenario_locale(long k) {
  arm(k);
  std::locale l;  // the first locale object of the process
  std::locale l2;
  disarm();
  EXPECT(l == std::locale::classic());
  EXPECT(l2 == l);
  EXPECT(std::use_facet<std::numpunct<char>>(l).decimal_point() == '.');
  EXPECT(std::use_facet<std::ctype<char>>(l).toupper('a') == 'A');
  return 0;
}

static int scenario_categories(long k) {
  arm(k);
  const std::error_category& g = std::generic_category();
  const std::error_category& s = std::system_category();
  const std::error_category& io = std::iostream_category();
  const std::error_category& fu = std::future_category();
  const char* gn = g.name();
  const char* sn = s.name();
  const char* ion = io.name();
  const char* fun = fu.name();
  const std::error_code none;
  const std::error_code inval = std::make_error_code(std::errc::invalid_argument);
  const std::error_code ioerr = std::make_error_code(std::io_errc::stream);
  const std::error_code ferr = std::make_error_code(std::future_errc::no_state);
  const std::error_condition zero = s.default_error_condition(0);
  const bool eq = inval == std::errc::invalid_argument && none == std::error_condition() && !(inval == ioerr);
  disarm();
  EXPECT(&g == &std::generic_category() && &s == &std::system_category());
  EXPECT(&io == &std::iostream_category() && &fu == &std::future_category());
  EXPECT(std::strcmp(gn, "generic") == 0 && std::strcmp(sn, "system") == 0);
  EXPECT(std::strcmp(ion, "iostream") == 0 && std::strcmp(fun, "future") == 0);
  EXPECT(none.value() == 0 && &none.category() == &s);
  EXPECT(&inval.category() == &g && inval.value() == EINVAL);
  EXPECT(&ioerr.category() == &io && &ferr.category() == &fu);
  EXPECT(zero.value() == 0 && &zero.category() == &g);
  EXPECT(eq);
  return 0;
}

struct Counting : std::pmr::memory_resource {
  void* do_allocate(std::size_t, std::size_t) override { return nullptr; }
  void do_deallocate(void*, std::size_t, std::size_t) override {}
  bool do_is_equal(const memory_resource& o) const noexcept override { return this == &o; }
};

static int scenario_resources(long k) {
  static Counting mine;
  arm(k);
  std::pmr::memory_resource* d = std::pmr::get_default_resource();
  std::pmr::memory_resource* nd = std::pmr::new_delete_resource();
  std::pmr::memory_resource* nu = std::pmr::null_memory_resource();
  std::pmr::memory_resource* prev = std::pmr::set_default_resource(&mine);
  std::pmr::memory_resource* now = std::pmr::get_default_resource();
  std::pmr::memory_resource* back = std::pmr::set_default_resource(nullptr);
  const bool same = nd->is_equal(*nd) && !nd->is_equal(*nu);
  disarm();
  EXPECT(d == nd && prev == nd && now == &mine && back == &mine);
  EXPECT(std::pmr::get_default_resource() == std::pmr::new_delete_resource());
  EXPECT(nd == std::pmr::new_delete_resource() && nu == std::pmr::null_memory_resource());
  EXPECT(nd != nu && same);
  return 0;
}

static int scenario_exception_state(long k) {
  // The thread is created before the failures start; its first exception-related calls run
  // while they happen.
  int result = -1;
  bool go = false;
  std::thread t([&] {
    while (!__atomic_load_n(&go, __ATOMIC_ACQUIRE)) {
    }
    int ok = 0;
    ok += std::uncaught_exceptions() == 0;
    ok += std::current_exception() == nullptr;
    ok += std::get_terminate() != nullptr;
    ok += std::this_thread::get_id() != std::thread::id();
    std::exception_ptr p;
    try {
      throw 42;
    } catch (int) {
      p = std::current_exception();
      ok += std::uncaught_exceptions() == 0;
    }
    ok += p != nullptr;
    __atomic_store_n(&result, ok, __ATOMIC_RELEASE);
    __atomic_store_n(&go, false, __ATOMIC_RELEASE);
    while (!__atomic_load_n(&go, __ATOMIC_ACQUIRE)) {
    }
    // (memory available again) the exception_ptr refers to the int or to a bad_alloc.
    try {
      std::rethrow_exception(p);
    } catch (int v) {
      if (v != 42) result = -2;
    } catch (const std::bad_alloc&) {
    } catch (...) {
      result = -3;
    }
  });
  arm(k);
  __atomic_store_n(&go, true, __ATOMIC_RELEASE);
  while (__atomic_load_n(&go, __ATOMIC_ACQUIRE)) {
  }
  disarm();
  __atomic_store_n(&go, true, __ATOMIC_RELEASE);
  t.join();
  EXPECT(result == 6);
  return 0;
}

static int scenario_random_device(long k) {
  std::random_device rd;  // (its constructor may throw: made before the failures start)
  arm(k);
  const double e = rd.entropy();
  disarm();
  EXPECT(e >= 0 && e <= std::numeric_limits<std::random_device::result_type>::digits);
  return 0;
}

static int scenario_clocks(long k) {
  arm(k);
  const auto s1 = std::chrono::system_clock::now();
  const auto t1 = std::chrono::steady_clock::now();
  const auto f1 = std::chrono::file_clock::now();
  const auto h1 = std::chrono::high_resolution_clock::now();
  const auto t2 = std::chrono::steady_clock::now();
  const auto f2 = std::chrono::file_clock::now();
  const bool dbg = std::is_debugger_present();
  std::breakpoint_if_debugging();  // (no debugger: no effect)
  const unsigned hc = std::thread::hardware_concurrency();
  const std::thread::id id = std::this_thread::get_id();
  disarm();
  EXPECT(t2 >= t1);
  EXPECT(s1.time_since_epoch().count() != 0 || f1.time_since_epoch().count() != 0 || h1.time_since_epoch().count() != 0);
  (void)f2;  // (file_clock need not be steady: only the calls themselves are checked)
  (void)dbg;
  (void)hc;  // ("If this value is not computable or well-defined, an implementation should return 0")
  EXPECT(id == std::this_thread::get_id() && id != std::thread::id());
  return 0;
}

static void sweep(const char* name, int (*scenario)(long)) {
  for (long k = 1; k <= 2000; ++k) {
    const pid_t pid = fork();
    CHECK(pid >= 0);
    if (pid == 0) {
      scenario(k);
      _exit(failed ? 0 : 10);
    }
    int status = 0;
    CHECK(waitpid(pid, &status, 0) == pid);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 10) return;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
      dprintf(2, "%s: k=%ld: %s %d\n", name, k, WIFEXITED(status) ? "exit" : "signal",
              WIFEXITED(status) ? WEXITSTATUS(status) : WTERMSIG(status));
      abort();
    }
  }
  dprintf(2, "%s: never completed\n", name);
  abort();
}

int main() {
  // Nothing in this process uses the library's lazily initialized state before the children
  // are forked.
  sweep("locale()", scenario_locale);
  sweep("error categories", scenario_categories);
  sweep("memory resources", scenario_resources);
  sweep("exception state in a new thread", scenario_exception_state);
  sweep("random_device::entropy", scenario_random_device);
  sweep("clocks", scenario_clocks);
  return 0;
}
