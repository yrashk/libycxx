// The first uses of the localization library in a process happen in several threads at once:
// [res.on.data.races]: library functions called from different threads on distinct objects
// (or only through const access) do not introduce data races; [locale.cons] locale() is a
// copy of the global locale (classic() unless global() was called); [locale.statics]
// classic() is "the "C" locale"; [locale.id]: each facet interface's id identifies it, so
// use_facet<F>/has_facet<F> ([locale.global.templates]) find exactly the facet of type F that
// the locale contains, even when the ids of two program-defined facets are first used by
// different threads simultaneously; [facet.num.put.virtuals]/[facet.num.get.virtuals] format
// and parse with the classic numpunct.
// Each child process starts its threads before any locale, facet or stream is used (no
// <iostream>), releases them together, and checks every result.
// FLAGS: -pthread
#include <atomic>
#include <locale>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "child_process.hpp"
#include "check.hpp"

struct F1 : std::locale::facet {
  static std::locale::id id;
  int k;
  explicit F1(int x) : k(x) {}
};
std::locale::id F1::id;
struct F2 : std::locale::facet {
  static std::locale::id id;
  int k;
  explicit F2(int x) : k(x) {}
};
std::locale::id F2::id;
struct F3 : std::locale::facet {
  static std::locale::id id;
};
std::locale::id F3::id;

static std::atomic<bool> go{false};
static std::atomic<int> failures{0};

static void work(int t) {
  while (!go.load(std::memory_order_acquire)) {
  }
  for (int round = 0; round < 50; ++round) {
    const int k = t * 1000 + round;
    std::locale base = (t + round) % 2 ? std::locale() : std::locale::classic();
    std::locale l1(base, new F1(k));
    std::locale l2(l1, new F2(-k));
    if (!std::has_facet<F1>(l1) || std::has_facet<F2>(l1) || std::has_facet<F3>(l2)) ++failures;
    if (std::use_facet<F1>(l2).k != k || std::use_facet<F2>(l2).k != -k) ++failures;
    if (std::has_facet<F1>(base) || std::has_facet<F2>(base)) ++failures;
    if (!(base == std::locale::classic()) || base.name() != "C") ++failures;
    const auto& ct = std::use_facet<std::ctype<char>>(l2);
    if (ct.toupper('q') != 'Q' || !ct.is(std::ctype_base::digit, '7')) ++failures;
    if (std::use_facet<std::numpunct<char>>(l2).decimal_point() != '.') ++failures;
    std::ostringstream os;
    os << 1234567 << ' ' << 2.5 << ' ' << std::boolalpha << true;
    if (os.str() != "1234567 2.5 true") ++failures;
    std::istringstream is("-42 0.125");
    long a = 0;
    double b = 0;
    is >> a >> b;
    if (a != -42 || b != 0.125) ++failures;
    std::wostringstream ws;
    ws << 77;
    if (ws.str() != L"77") ++failures;
  }
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  if (child_mode()) {
    std::vector<std::thread> ts;
    for (int t = 0; t < 8; ++t) ts.emplace_back(work, t);
    go.store(true, std::memory_order_release);
    for (auto& th : ts) th.join();
    return failures.load() == 0 ? 0 : 1;
  }
  for (int i = 0; i < 6; ++i) {
    ChildResult r = run_self("child");
    if (r.status != 0) dprintf(2, "child %d: status %d\n%s\n", i, r.status, r.err.c_str());
    CHECK(r.status == 0);
  }
}
