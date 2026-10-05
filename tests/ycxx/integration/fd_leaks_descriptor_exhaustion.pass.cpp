// Library operations that need several file descriptors at once, run when only 0 to 4
// descriptors are free: an operation that gets some of the descriptors it needs and then fails
// to open the next one (EMFILE) reports the failure and closes what it opened.
//   [fs.err.report]/3: an operation with an error_code& argument reports an operating system
//     error through it, one without throws filesystem_error; [res.on.exception.handling]/1 and
//     the ownership of directory streams by directory iterators ([fs.class.directory.iterator],
//     [fs.class.rec.dir.itr]) and of files by file buffers ([filebuf.cons]/5): nothing the
//     failed operation opened stays open.
//   [rand.device]/4,/7: random_device's constructor and operator() throw an exception derived
//     from exception if the device cannot be initialized or a number cannot be obtained.
//   [time.zone.db.access]: get_tzdb throws runtime_error if no valid database can be returned.
//   [stacktrace.basic.obs], [stacktrace.entry.query]: capturing and describing a stack trace
//     report no error; the descriptions may be empty.
// Each run is a fresh child process (first uses of the time zone database and of stack trace
// symbolization included): it fills the descriptor table up to the given number of free slots,
// makes the operation, closes the fillers, makes the operation again with no limit, and counts
// its descriptors; a run with no filling gives the expected count (descriptors an operation
// keeps open on purpose, such as a cache, appear in both). A child killed by a signal or by
// std::terminate also fails.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <stacktrace>
#include <string>
#include <system_error>
#include <vector>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"
#include "fs_tmpdir.hpp"

namespace fs = std::filesystem;

static int open_fds() {
  int n = 0;
  for (int fd = 0; fd < 1024; ++fd) n += fcntl(fd, F_GETFD) != -1;
  return n;
}

static std::string base;

static void make_tree(const std::string& root, int depth) {
  mkdir(root.c_str(), 0700);
  std::string d = root;
  for (int i = 0; i < depth; ++i) {
    std::ofstream(d + "/file") << "contents";
    d += "/level" + std::to_string(i);
    mkdir(d.c_str(), 0700);
  }
}

static void op_copy_file_ec() {
  std::error_code ec;
  fs::copy_file(base + "/tree/file", base + "/copied", fs::copy_options::overwrite_existing, ec);
}
static void op_copy_file() { fs::copy_file(base + "/tree/file", base + "/copied", fs::copy_options::overwrite_existing); }
static void op_copy_recursive_ec() {
  std::error_code ec;
  fs::copy(base + "/tree", base + "/tree-copy", fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
}
static void op_copy_recursive() {
  fs::copy(base + "/tree", base + "/tree-copy2", fs::copy_options::recursive | fs::copy_options::overwrite_existing);
}
static void op_remove_all_ec() {
  make_tree(base + "/doomed", 6);
  std::error_code ec;
  fs::remove_all(base + "/doomed", ec);
}
static void op_remove_all() {
  make_tree(base + "/doomed2", 6);
  fs::remove_all(base + "/doomed2");
}
static void op_walk_ec() {
  std::error_code ec;
  fs::recursive_directory_iterator it(base + "/tree", ec), end;
  for (int n = 0; !ec && it != end && n < 100; ++n) it.increment(ec);
}
static void op_walk() {
  for (auto it = fs::recursive_directory_iterator(base + "/tree"); it != fs::recursive_directory_iterator(); ++it) {
  }
}
static void op_streams() {
  std::ifstream in(base + "/tree/file");
  std::ofstream out(base + "/stream-copy");
  std::fstream both(base + "/stream-both", std::ios::in | std::ios::out | std::ios::trunc);
  out << in.rdbuf();
  both << "x";
}
static void op_random_device() {
  std::random_device a, b;
  (void)a();
  (void)b();
}
static void op_tzdb() {
  const auto& db = std::chrono::get_tzdb();
  (void)db.locate_zone("Europe/Paris");
  (void)std::chrono::current_zone();
}
static void op_stacktrace() {
  const std::stacktrace st = std::stacktrace::current();
  (void)std::to_string(st);
}

struct Op {
  const char* name;
  void (*f)();
};
static const Op ops[] = {
    {"copy_file(ec)", op_copy_file_ec},   {"copy_file", op_copy_file},     {"copy recursive(ec)", op_copy_recursive_ec},
    {"copy recursive", op_copy_recursive}, {"remove_all(ec)", op_remove_all_ec}, {"remove_all", op_remove_all},
    {"walk(ec)", op_walk_ec},             {"walk", op_walk},               {"file streams", op_streams},
    {"random_device", op_random_device},  {"time zone database", op_tzdb}, {"stacktrace", op_stacktrace},
};

static void attempt(void (*f)()) {
  try {
    f();
  } catch (...) {
  }
}

// In a child: fill the table leaving `free_slots` free (-1: no filling), make the operation,
// close the fillers, make it again, return the number of open descriptors.
static int child(void (*f)(), int free_slots) {
  std::vector<int> fillers;
  if (free_slots >= 0) {
    rlimit lim{};
    getrlimit(RLIMIT_NOFILE, &lim);
    lim.rlim_cur = 128;  // a small table, so filling it is quick
    if (setrlimit(RLIMIT_NOFILE, &lim) != 0) return 126;
    for (;;) {
      const int fd = open("/dev/null", O_RDONLY);
      if (fd < 0) break;
      fillers.push_back(fd);
    }
    for (int i = 0; i < free_slots && !fillers.empty(); ++i) {
      close(fillers.back());
      fillers.pop_back();
    }
  }
  attempt(f);
  for (int fd : fillers) close(fd);
  attempt(f);
  const int n = open_fds();
  return n < 120 ? n : 120;
}

static int run_child(void (*f)(), int free_slots) {
  const pid_t pid = fork();
  CHECK(pid >= 0);
  if (pid == 0) _exit(child(f, free_slots));
  int status = 0;
  CHECK(waitpid(pid, &status, 0) == pid);
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

int main() {
  TmpDir tmp;
  base = tmp.str();
  make_tree(base + "/tree", 5);
  int failures = 0;
  for (const Op& op : ops) {
    const int expected = run_child(op.f, -1);
    CHECK(expected > 0 && expected < 120);
    for (int free_slots = 0; free_slots <= 4; ++free_slots) {
      const int r = run_child(op.f, free_slots);
      if (r < 0) {
        ++failures;
        dprintf(2, "%s with %d free descriptor(s): the process was killed\n", op.name, free_slots);
      } else if (r != expected) {
        ++failures;
        dprintf(2, "%s with %d free descriptor(s): %d descriptor(s) left open\n", op.name, free_slots, r - expected);
      }
    }
  }
  CHECK(failures == 0);
  return 0;
}
