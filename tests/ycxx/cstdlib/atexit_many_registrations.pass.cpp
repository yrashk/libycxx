// atexit registrations: at least 32, beyond that each call reports success or failure, from
// many threads at once, interleaved with static objects; run in child processes.
//   [support.start.term]/6: atexit registers f "to be called without arguments at normal
//     program termination"; Note 2: "The atexit() functions do not introduce a data race".
//   /7: "The implementation shall support the registration of at least 32 functions."
//   /8: "Returns: zero if the registration succeeds, nonzero if it fails."
//   [basic.start.term]/5 (via /9.1 exit): "If a call to std::atexit is sequenced before the
//     completion of the initialization of an object with static storage duration, the call to
//     the function passed to std::atexit is sequenced before the call to the destructor for the
//     object"; functions are called in the reverse order of their registration ([basic.start.term]
//     /6 with the C standard's atexit: "in the reverse order of their registration").
//   [support.start.term]/10-13: at_quick_exit registrations are distinct from these.
// Every function whose registration returned zero is called exactly once, in reverse order of
// registration (per thread: registrations of one thread are sequenced).
// FLAGS: -pthread
#include <cstdlib>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <string.h>
#include <unistd.h>
#include "check.hpp"
#include "child_process.hpp"

static void say(const char* s) { (void)!write(1, s, strlen(s)); }

namespace {
template<int N> void numbered() {
  char buf[16];
  int n = N, i = 15;
  buf[i--] = 0;
  buf[i--] = ' ';
  do {
    buf[i--] = char('0' + n % 10);
    n /= 10;
  } while (n);
  say(buf + i + 1);
}
template<int... N> int register_numbered(std::integer_sequence<int, N...>) {
  int ok = 0;
  ((ok += std::atexit(numbered<N>) == 0), ...);
  return ok;
}

struct Named {
  const char* text;
  ~Named() { say(text); }
};

std::vector<int>* thread_counts = nullptr;
template<int T> void per_thread() {
  // The counter is not protected: exit calls the functions on one thread, after the threads
  // were joined.
  ++(*thread_counts)[T];
}
}  // namespace

extern "C" void c_handler() { say("c "); }

static int child(const std::string& mode) {
  if (mode == "limit32") {
    if (register_numbered(std::make_integer_sequence<int, 32>()) != 32) return 101;
    std::exit(0);
  }
  if (mode == "many") {
    // 2000 registrations of 500 functions (each registered four times): successes are counted
    // and every one of them must be called.
    int ok = 0;
    for (int round = 0; round < 4; ++round) ok += register_numbered(std::make_integer_sequence<int, 500>());
    char buf[32];
    const int n = snprintf(buf, sizeof buf, "%d\n", ok);
    (void)!write(2, buf, static_cast<std::size_t>(n));
    std::exit(0);
  }
  if (mode == "interleaved") {
    if (std::atexit(c_handler) != 0) return 102;
    static Named a{"a "};
    if (std::atexit(numbered<1>) != 0) return 103;
    static Named b{"b "};
    if (std::atexit(numbered<2>) != 0) return 104;
    return 0;  // returning from main calls exit
  }
  if (mode == "threads") {
    static std::vector<int> counts(8);  // constructed before the registrations: destroyed after them
    thread_counts = &counts;
    // Registered first, so called last: reports the calls the per-thread functions made.
    if (std::atexit([] {
          std::string s;
          for (int v : *thread_counts) s += std::to_string(v) + ' ';
          say(s.c_str());
        }) != 0)
      return 105;
    std::vector<std::thread> ts;
    std::vector<int> ok(8);
    auto start = [&]<int... T>(std::integer_sequence<int, T...>) {
      (ts.emplace_back([&ok] {
        for (int i = 0; i < 100; ++i) ok[T] += std::atexit(per_thread<T>) == 0;
      }), ...);
    };
    start(std::make_integer_sequence<int, 8>());
    for (auto& t : ts) t.join();
    std::string s;  // the successful registrations, written now
    for (int v : ok) s += std::to_string(v) + ' ';
    s += "| ";
    say(s.c_str());
    std::exit(0);
  }
  return 110;
}

int main(int argc, char** argv) {
  if (child_mode()) return child(argc > 1 ? argv[1] : "");
  ChildResult r = run_self("limit32");
  std::string want;
  for (int i = 31; i >= 0; --i) want += std::to_string(i) + ' ';
  CHECK(same_text(r.out, want, "limit32 stdout"));
  CHECK(r.status == 0 && r.err.empty());

  r = run_self("many");
  CHECK(r.status == 0);
  const int ok = std::atoi(r.err.c_str());
  CHECK(ok >= 32 && ok <= 2000);
  // The successful registrations are a prefix of the attempts (the first 32 at least succeed;
  // a later failure means the table is full, and nothing guarantees a later attempt fails too,
  // so the expected output is built from the count only when all succeeded).
  if (ok == 2000) {
    want.clear();
    for (int round = 0; round < 4; ++round)
      for (int i = 499; i >= 0; --i) want += std::to_string(i) + ' ';
    CHECK(same_text(r.out, want, "many stdout"));
  } else {
    int words = 0;
    for (char c : r.out) words += c == ' ';
    CHECK(words == ok);
  }

  r = run_self("interleaved");
  CHECK(same_text(r.out, "2 b 1 a c ", "interleaved stdout"));
  CHECK(r.status == 0 && r.err.empty());

  r = run_self("threads");
  CHECK(r.status == 0 && r.err.empty());
  const std::size_t bar = r.out.find("| ");
  CHECK(bar != std::string::npos);
  CHECK(same_text(r.out.substr(bar + 2), r.out.substr(0, bar), "per-thread calls vs successful registrations"));
  return 0;
}
