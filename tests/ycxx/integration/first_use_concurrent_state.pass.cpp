// The first uses of library state with program-wide identity happen in many threads at once
// (each scenario in a fresh child process, its threads started before the library state is
// touched and released together):
//   [syserr.errcat.objects]/1, /3: generic_category() and system_category(): "All calls to
//     this function shall return references to the same object"; [iostreams.base]
//     iostream_category() and [futures.errors] future_category() likewise (each returns "a
//     reference to an object of a type derived from class error_category", the same one: the
//     categories compare by address, [syserr.errcat.overview]/1); their name()s;
//     error_code::message() of the same code from every thread is the same string
//     ([syserr.errcat.virtuals]: message(ev) is a function of ev).
//   [mem.res.global]/1-2: new_delete_resource() and null_memory_resource(): "The same value is
//     returned every time this function is called"; /3: the default resource is initially
//     new_delete_resource(); /6: "Calling the set_default_resource and get_default_resource
//     functions shall not incur a data race. A call to the set_default_resource function
//     synchronizes with subsequent calls to the set_default_resource and get_default_resource
//     functions": with every thread installing its own resource once, each get_default_resource
//     returns one of the installed values or the initial one, and the values that the
//     set_default_resource calls return, together with the final value, are the initial one and
//     each installed resource exactly once (each call replaces exactly the previous value).
//   [stacktrace.entry.query]/2-3, [stacktrace.entry.obs]: description(), source_file() and
//     source_line() of one stacktrace_entry, called concurrently on the same const object
//     ([res.on.data.races]/3), give the same results in every thread as when called alone.
//   [locale.statics]/4-5 classic(): every thread gets a locale equal to locale("C") whose
//     facets are classic.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <atomic>
#include <cstring>
#include <future>
#include <ios>
#include <locale>
#include <memory_resource>
#include <stacktrace>
#include <string>
#include <system_error>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"

constexpr int N = 8;

// Runs body(i) in N threads released together.
template <class F>
static void together(F body) {
  std::atomic<int> ready{0};
  std::atomic<bool> go{false};
  std::thread ts[N];
  for (int i = 0; i < N; ++i)
    ts[i] = std::thread([&, i] {
      ready.fetch_add(1);
      while (!go.load()) {
      }
      body(i);
    });
  while (ready.load() != N) {
  }
  go = true;
  for (auto& t : ts) t.join();
}

static void categories() {
  const std::error_category* cat[N][4] = {};
  const char* names[N][4] = {};
  std::string msgs[N];
  together([&](int i) {
    cat[i][0] = &std::generic_category();
    cat[i][1] = &std::system_category();
    cat[i][2] = &std::iostream_category();
    cat[i][3] = &std::future_category();
    for (int j = 0; j < 4; ++j) names[i][j] = cat[i][j]->name();
    msgs[i] = std::make_error_code(std::errc::no_such_file_or_directory).message();
  });
  const char* want[4] = {"generic", "system", "iostream", "future"};
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < 4; ++j) {
      CHECK(cat[i][j] == cat[0][j]);
      CHECK(std::strcmp(names[i][j], want[j]) == 0);
    }
    CHECK(msgs[i] == msgs[0]);
  }
  CHECK(cat[0][0] == &std::generic_category() && cat[0][1] == &std::system_category());
  CHECK(cat[0][2] == &std::iostream_category() && cat[0][3] == &std::future_category());
  CHECK(cat[0][0] != cat[0][1]);
  CHECK(msgs[0] == std::generic_category().message(ENOENT));
}

struct Mine : std::pmr::memory_resource {
  void* do_allocate(std::size_t, std::size_t) override { throw std::bad_alloc(); }
  void do_deallocate(void*, std::size_t, std::size_t) override {}
  bool do_is_equal(const memory_resource& o) const noexcept override { return this == &o; }
};

static void resources() {
  static Mine mine[N];
  std::pmr::memory_resource* nd[N] = {};
  std::pmr::memory_resource* nu[N] = {};
  std::pmr::memory_resource* seen[N] = {};
  std::pmr::memory_resource* prev[N] = {};
  together([&](int i) {
    nd[i] = std::pmr::new_delete_resource();
    nu[i] = std::pmr::null_memory_resource();
    seen[i] = std::pmr::get_default_resource();
    prev[i] = std::pmr::set_default_resource(&mine[i]);
  });
  std::pmr::memory_resource* const final_value = std::pmr::get_default_resource();
  for (int i = 0; i < N; ++i) {
    CHECK(nd[i] == nd[0] && nu[i] == nu[0]);
    bool known = seen[i] == nd[0];
    for (auto& m : mine) known = known || seen[i] == &m;
    CHECK(known);
  }
  CHECK(nd[0] == std::pmr::new_delete_resource() && nu[0] == std::pmr::null_memory_resource() && nd[0] != nu[0]);
  // The chain of replacements: the previous values and the final one are {initial} + mine[*].
  int count_initial = (final_value == nd[0]);
  int count_mine[N] = {};
  for (int i = 0; i < N; ++i) {
    count_initial += prev[i] == nd[0];
    for (int j = 0; j < N; ++j) count_mine[j] += prev[i] == &mine[j];
  }
  for (int j = 0; j < N; ++j) count_mine[j] += final_value == &mine[j];
  CHECK(count_initial == 1);
  for (int j = 0; j < N; ++j) CHECK(count_mine[j] == 1);
}

[[gnu::noinline]] static std::stacktrace capture() { return std::stacktrace::current(); }

static void stacktrace_queries() {
  const std::stacktrace st = capture();
  if (st.empty()) return;  // (no stack trace available: nothing to compare)
  const std::stacktrace_entry& e = st[0];
  std::string desc[N], file[N];
  std::uint_least32_t line[N] = {};
  together([&](int i) {
    desc[i] = e.description();
    file[i] = e.source_file();
    line[i] = e.source_line();
  });
  for (int i = 0; i < N; ++i) CHECK(desc[i] == desc[0] && file[i] == file[0] && line[i] == line[0]);
  CHECK(e.description() == desc[0] && e.source_file() == file[0] && e.source_line() == line[0]);
}

// The first captures and symbolizations of the process, in every thread at once: each thread's
// trace of the same call path has the same entries ([stacktrace.basic.cons]/1: current()
// returns the stacktrace of the current evaluation, or an empty one if it cannot), and the
// same entry describes alike in every thread ([stacktrace.entry.query]).
[[gnu::noinline]] static std::stacktrace capture_here(int) { return std::stacktrace::current(); }

static void stacktrace_capture() {
  std::stacktrace traces[N];
  std::string first_desc[N];
  together([&](int i) {
    traces[i] = capture_here(i);
    if (!traces[i].empty()) first_desc[i] = traces[i][0].description();
  });
  for (int i = 0; i < N; ++i) {
    if (traces[i].empty() || traces[0].empty()) continue;  // (no trace available in that thread)
    CHECK(traces[i][0] == traces[0][0]);
    CHECK(first_desc[i] == first_desc[0]);
  }
}

static void classic_locale() {
  bool ok[N] = {};
  together([&](int i) {
    const std::locale& c = std::locale::classic();
    ok[i] = c.name() == "C" && std::use_facet<std::numpunct<char>>(c).decimal_point() == '.' &&
            std::use_facet<std::ctype<char>>(c).is(std::ctype_base::alpha, 'x');
  });
  for (bool b : ok) CHECK(b);
  CHECK(std::locale::classic() == std::locale("C"));
}

static void in_child(void (*f)(), const char* name) {
  const pid_t pid = fork();
  CHECK(pid >= 0);
  if (pid == 0) {
    f();
    _exit(0);
  }
  int status = 0;
  CHECK(waitpid(pid, &status, 0) == pid);
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) dprintf(2, "%s failed (status %#x)\n", name, status);
  CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

int main() {
  in_child(categories, "error categories");
  in_child(resources, "memory resources");
  in_child(stacktrace_queries, "stacktrace queries");
  in_child(stacktrace_capture, "stacktrace captures");
  in_child(classic_locale, "classic locale");
  return 0;
}
