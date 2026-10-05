// std::exit called by a thread other than main while main and another thread are blocked
// (outside the library, in read(2)): the termination sequence runs on the calling thread.
//   [support.start.term]/9.1: "First, objects with thread storage duration and associated with
//     the current thread are destroyed. Next, objects with static storage duration are destroyed
//     and functions registered by calling atexit are called." /9.2: "all open C streams ... with
//     unwritten buffered data are flushed". /9.3: the status is returned to the host.
//   [basic.start.term]/2: thread-storage objects are destroyed "as a result of that thread
//     calling std::exit" -- only that thread's: main's thread_local object is not destroyed;
//     their destruction "strongly happens before destroying any object with static storage
//     duration". /6: an atexit function registered after the completion of a static object's
//     initialization is called before that object's destruction.
//   [basic.start.term]/7: the library may be used during the sequence (no other thread uses it).
//   [iostream.objects.overview]/3: cout is not destroyed; its pending output is written
//     ([ios.init]: ~Init flushes cout; with sync_with_stdio, cout writes through stdout, which
//     /9.2 flushes).
//   [filebuf.cons]/5, [ofstream.cons]: the destructor of a static ofstream closes its file,
//     writing its buffered characters ([filebuf.members] close: "calls overflow(traits::eof())").
// FLAGS: -pthread
#include <cstdio>
#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <locale>
#include <memory_resource>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "child_process.hpp"
#include "check.hpp"

static std::string lib_use() {
  std::ostringstream os;
  os.imbue(std::locale::classic());
  os << std::format("{:05.1f}", 2.25) << ' ' << std::hex << 255;
  std::pmr::vector<std::string> v(std::pmr::get_default_resource());
  v.emplace_back("a string long enough to be allocated on the heap");
  return os.str() + (v.size() == 1 ? " ok" : " bad");
}

namespace {
struct Early {
  // The constructor uses nothing from the library.
  ~Early() {
    if (child_mode()) std::cout << "static " << lib_use() << '\n';
  }
} early;

std::ofstream* file_ptr;
struct FileHolder {
  std::ofstream f;
  FileHolder() {
    if (const char* p = getenv("YCXX_TEST_FILE")) {
      f.open(p);
      file_ptr = &f;
    }
  }
} file_holder;

struct MainTL {
  ~MainTL() { std::cout << "main-tl\n"; }
};
struct ExitingTL {
  ~ExitingTL() { std::cout << "exiting-tl " << lib_use() << '\n'; }
};

void at_exit_fn() { std::printf("atexit %s\n", lib_use().c_str()); }

void block_forever() {
  int p[2];
  if (pipe(p) != 0) std::abort();
  char c;
  (void)!read(p[0], &c, 1);
}
}  // namespace

int main() {
  if (child_mode()) {
    thread_local MainTL main_tl;
    (void)&main_tl;
    std::cout << std::unitbuf << "start\n" << std::nounitbuf;
    if (std::atexit(at_exit_fn) != 0) return 101;
    std::thread blocked(block_forever);
    std::thread exiting([] {
      thread_local ExitingTL tl;
      (void)&tl;
      *file_ptr << "file contents " << 42;  // buffered in the filebuf
      std::cout << "pending ";              // no flush
      std::exit(5);
    });
    block_forever();
    return 102;
  }
  char path[] = "/tmp/ycxx-exit-thread-XXXXXX";
  int fd = mkstemp(path);
  CHECK(fd >= 0);
  close(fd);
  setenv("YCXX_TEST_FILE", path, 1);
  ChildResult r = run_self("run");
  CHECK(same_text(r.out, "start\npending exiting-tl 002.2 ff ok\natexit 002.2 ff ok\nstatic 002.2 ff ok\n", "stdout"));
  CHECK(r.status == 5);
  CHECK(r.err.empty());
  std::ifstream in(path);
  std::string got;
  std::getline(in, got);
  unlink(path);
  CHECK(same_text(got, "file contents 42", "file"));
  return 0;
}
