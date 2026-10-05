// Standard-library facilities used during program termination: in the destructor of a static
// object constructed before main (whose constructor touches no library facility, so any
// library-internal static object created lazily during main is constructed after it and, by
// [basic.start.term]/4, destroyed before it), and in an atexit function registered before main
// (called after the destruction of everything constructed after its registration:
// [basic.start.term]/6).
//   [basic.start.term]/7: undefined behaviour arises only for a use of a library facility that
//     does not happen before the completion of destruction of objects with static storage
//     duration and of the atexit calls; these uses happen during that sequence, before its
//     completion, so they are valid and must behave as specified. (Its note: "These
//     requirements permit thread managers as static-storage-duration objects.")
//   [iostream.objects.overview]/3: cout is not destroyed.
//   [mem.res.global]/1-2: new_delete_resource() and get_default_resource() return pointers to
//     objects that the program may use (no end of lifetime is specified for them).
// The same battery runs during dynamic initialisation, in main (to create any lazy internal
// state), in the destructor and in the atexit function; the program re-runs itself in a child process and checks its exact
// output and exit status.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <functional>
#include <iostream>
#include <locale>
#include <memory_resource>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <vector>
#include "child_process.hpp"
#include "check.hpp"

static std::string battery() {
  std::string r;
  auto mark = [&](bool ok) { r += ok ? '.' : 'X'; };
  mark(std::format("{} {:.3f} {:>5}|{:x}", 42, 3.14159, "x", 255) == "42 3.142     x|ff");
  mark(std::format(std::locale::classic(), "{:L}", 1234567) == "1234567");
  std::locale loc;
  mark(std::use_facet<std::ctype<char>>(loc).toupper('a') == 'A');
  mark(std::isdigit('7', loc) && !std::isalpha('7', loc));
  std::ostringstream os;
  os << 1.5 << ' ' << std::boolalpha << true << ' ' << std::hex << 255;
  mark(os.str() == "1.5 true ff");
  std::istringstream is("12 abc\nline2");
  int n = 0;
  std::string w, line;
  is >> n >> w;
  std::getline(is >> std::ws, line);
  mark(n == 12 && w == "abc" && line == "line2");
  mark(std::regex_match("abc123", std::regex("[a-z]+[0-9]+")) && !std::regex_search("ABC", std::regex("[a-z]")));
  mark(!std::generic_category().message(EDOM).empty());
  mark(std::system_category().default_error_condition(EDOM) == std::errc::argument_out_of_domain);
  mark(std::make_error_code(std::errc::invalid_argument).category() == std::generic_category());
  char buf[32];
  auto [p, ec] = std::to_chars(buf, buf + sizeof buf, 0.1);
  mark(ec == std::errc() && std::string(buf, p) == "0.1");
  mark(std::stod("2.5") == 2.5 && std::to_string(7) == "7");
  mark(std::filesystem::path("a/b.txt").filename() == "b.txt" && std::filesystem::path("a/b.txt").extension() == ".txt");
  try {
    throw std::system_error(std::make_error_code(std::errc::io_error), "ctx");
  } catch (const std::system_error& e) {
    mark(e.code() == std::errc::io_error && std::string(e.what()).find("ctx") != std::string::npos);
  }
  std::exception_ptr ep = std::make_exception_ptr(std::runtime_error("ep"));
  try {
    std::rethrow_exception(ep);
  } catch (const std::runtime_error& e) {
    mark(std::string(e.what()) == "ep");
  }
  int from_thread = 0;
  std::jthread([&] { from_thread = 5; }).join();
  mark(from_thread == 5);
  std::random_device rd;
  (void)rd();
  mark(true);
  std::pmr::vector<std::string> pv(std::pmr::get_default_resource());
  pv.emplace_back("a long string that does not fit in the small buffer");
  void* q = std::pmr::new_delete_resource()->allocate(64, 16);
  std::pmr::new_delete_resource()->deallocate(q, 64, 16);
  mark(pv.size() == 1 && std::pmr::get_default_resource() == std::pmr::new_delete_resource());
  std::pmr::unsynchronized_pool_resource pool;
  std::pmr::vector<int> iv({1, 2, 3}, &pool);
  mark(iv.size() == 3);
  auto zone = std::chrono::locate_zone("UTC");
  mark(zone != nullptr && zone->get_info(std::chrono::sys_seconds{}).offset == std::chrono::seconds(0));  // "UTC" may be a link
  mark(std::format("{:%F %T}", std::chrono::sys_seconds(std::chrono::seconds(86400 * 365))) == "1971-01-01 00:00:00");
  std::function<int(int)> fn = [](int x) { return x * 2; };
  mark(fn(21) == 42);
  return r;
}

namespace {
struct Early {
  // Constructor: nothing from the library.
  ~Early() {
    if (!child_mode()) return;
    std::cout << "dtor " << battery() << std::endl;
  }
} early;

void at_exit_fn() {
  std::cout << "atexit " << battery() << std::endl;
}
struct Register {
  Register() {
    if (getenv("YCXX_CHILD_MODE")) atexit(at_exit_fn);  // C library atexit: before main
  }
} reg;
// Dynamic initialisation: the battery before main ([basic.start.dynamic]; this TU includes
// <iostream> first, so cout is usable: [iostream.objects.overview]/5).
struct Init {
  Init() {
    if (getenv("YCXX_CHILD_MODE")) std::cout << "init " << battery() << std::endl;
  }
} init;
}  // namespace

int main() {
  if (child_mode()) {
    std::cout << "main " << battery() << std::endl;
    return 0;
  }
  const std::string want(22, '.');
  ChildResult r = run_self("run");
  CHECK(same_text(r.out, "init " + want + "\nmain " + want + "\natexit " + want + "\ndtor " + want + "\n", "stdout"));
  CHECK(r.status == 0);
  CHECK(r.err.empty());
  return 0;
}
