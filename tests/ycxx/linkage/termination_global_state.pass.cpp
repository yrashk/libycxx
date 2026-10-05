// Program-wide library state set during main and used during termination, by the destructor of
// a static object constructed before main (constructor using nothing from the library) and by
// an atexit function registered before main ([basic.start.term]/4, /6: both run after
// everything the library may have created lazily during main has been destroyed, and before
// the termination completes). [basic.start.term]/7 makes only uses after the completion of
// that sequence undefined, so these uses must see the state main left:
//   [locale.statics]/2: the global locale set by locale::global (with a user facet and a
//     numpunct giving digit grouping) is what locale() returns and what a stream constructed
//     afterwards uses ([ios.base.locales]/4: getloc() is "a copy of the global C++ locale ...
//     in effect at the time of construction");
//   [ios.base.locales]: the locale imbued in cout stays in effect ([iostream.objects.overview]/3:
//     cout is not destroyed);
//   [set.terminate], [get.terminate]: get_terminate() returns the handler set in main;
//   [mem.res.global]/6-7: get_default_resource() returns the resource set by
//     set_default_resource in main (a user resource that outlives both uses);
//   [syserr.errcat.objects]: the category objects and their messages;
//   [time.zone.db.access]: get_tzdb() and current_zone();
//   [print.fun]: println to stdout (the C stream is flushed by exit, [support.start.term]/9.2);
//   wcout is used by the atexit function only (no orientation conflict with cout: they share
//     stdout, so the narrow and wide outputs are kept on different streams: wcerr for wide).
// FLAGS: -pthread
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <format>
#include <iostream>
#include <locale>
#include <memory_resource>
#include <print>
#include <sstream>
#include <string>
#include <system_error>
#include "child_process.hpp"
#include "check.hpp"

namespace {
struct Tag : std::locale::facet {
  static inline std::locale::id id;
  explicit Tag(std::size_t refs = 0) : std::locale::facet(refs) {}
  const char* tag() const { return "tagged"; }
};
struct Grouping : std::numpunct<char> {
  char do_thousands_sep() const override { return '\''; }
  std::string do_grouping() const override { return "\3"; }
};

// A resource defined before Early, so destroyed after it.
struct CountingResource : std::pmr::memory_resource {
  long n = 0;
  void* do_allocate(std::size_t b, std::size_t a) override {
    ++n;
    return std::pmr::new_delete_resource()->allocate(b, a);
  }
  void do_deallocate(void* p, std::size_t b, std::size_t a) override {
    std::pmr::new_delete_resource()->deallocate(p, b, a);
  }
  bool do_is_equal(const std::pmr::memory_resource& o) const noexcept override { return this == &o; }
} counting;

[[noreturn]] void my_terminate() { std::_Exit(99); }

std::string check_state(const char* who) {
  std::string r = who;
  std::locale g;
  r += std::has_facet<Tag>(g) ? std::string(" ") + std::use_facet<Tag>(g).tag() : " no-tag";
  std::ostringstream os;  // constructed now: the global locale
  os << 1234567;
  r += " " + os.str();
  r += std::get_terminate() == &my_terminate ? " terminate-ok" : " terminate-BAD";
  long before = counting.n;
  std::pmr::string s("a string long enough to come from the default memory resource");
  r += std::pmr::get_default_resource() == &counting && counting.n == before + 1 ? " resource-ok" : " resource-BAD";
  r += std::generic_category().message(ENOENT).empty() ? " errmsg-BAD" : " errmsg-ok";
  r += std::chrono::get_tzdb().locate_zone("UTC") != nullptr ? " tz-ok" : " tz-BAD";
  r += std::chrono::current_zone() != nullptr ? " zone-ok" : " zone-BAD";
  return r;
}

struct Early {
  // The constructor uses nothing from the library.
  ~Early() {
    if (!child_mode()) return;
    std::cout << check_state("dtor") << ' ' << 7654321 << '\n';  // cout's imbued grouping
  }
} early;

void at_exit_fn() {
  std::println(stdout, "{}", check_state("atexit"));
  std::wcerr << L"wide " << 42 << std::endl;
}
struct Register {
  Register() {
    if (getenv("YCXX_CHILD_MODE")) std::atexit(at_exit_fn);
  }
} reg;
}  // namespace

int main() {
  if (child_mode()) {
    std::locale::global(std::locale(std::locale(std::locale::classic(), new Tag), new Grouping));
    std::cout.imbue(std::locale(std::locale::classic(), new Grouping));
    std::set_terminate(my_terminate);
    std::pmr::set_default_resource(&counting);
    std::cout << check_state("main") << std::endl;
    return 0;
  }
  ChildResult r = run_self("run");
  const std::string tail = " tagged 1'234'567 terminate-ok resource-ok errmsg-ok tz-ok zone-ok";
  CHECK(same_text(r.out, "main" + tail + "\natexit" + tail + "\ndtor" + tail + " 7'654'321\n", "stdout"));
  CHECK(same_text(r.err, "wide 42\n", "stderr"));
  CHECK(r.status == 0);
  return 0;
}
