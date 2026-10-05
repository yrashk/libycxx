// Library operations that open files or directories (file streams, filesystem operations that
// copy, remove or walk trees, random_device, the time zone database on first access, stack
// trace symbolization) while operator new fails at its k-th call, for every k until the
// operation makes no allocation that fails. An operation that fails by an exception, or by
// reporting an error, keeps no file descriptor open ([res.on.exception.handling]/1: a failure
// is reported by the exception; the objects involved are destroyed and release what they own:
// [filebuf.cons]/5 ~basic_filebuf calls close(), [fs.class.directory.iterator] and
// [fs.class.rec.dir.itr] iterators own their directory stream).
// Each run is a fresh child process: it makes the operation with the failure armed, then once
// more with no failure, and counts its open descriptors; a run with no failure armed (made the
// same way) gives the expected count (descriptors an operation keeps open on purpose, such as
// a cache, appear in both).
// Which k are tried: the run with no failure armed records the calls of operator new made while
// the operation holds a descriptor it opened (one of the lowest descriptors that were free when
// it started is open). Every such k is tried, with its neighbours, and so is every k up to
// 200; the other k (no descriptor held: a failure there can leak only if the operation goes on
// after it) are sampled, about 200 per operation spread evenly. (The time zone database makes
// about 17000 allocations, of which only a handful happen while a file is open.)
// FLAGS: -pthread
// REQUIRES: exceptions
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <new>
#include <random>
#include <stacktrace>
#include <string>
#include <system_error>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"
#include "fs_tmpdir.hpp"

static long fail_at = 0, calls = 0;
static bool failed = false;
constexpr long max_recorded = 1 << 20;
static unsigned char* holding = nullptr;  // (shared with the parent) per call: a descriptor was held
static int probe_base = -1;               // the lowest descriptor free when the operation started
static void* allocate(std::size_t n, std::size_t align, bool nothrow) {
  if (probe_base >= 0 && calls + 1 < max_recorded) {
    for (int fd = probe_base; fd < probe_base + 16; ++fd)
      if (fcntl(fd, F_GETFD) != -1) {
        holding[calls + 1] = 1;
        break;
      }
  }
  if (++calls == fail_at) {
    failed = true;
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

namespace fs = std::filesystem;

static int open_fds() {
  int n = 0;
  for (int fd = 0; fd < 1024; ++fd) n += fcntl(fd, F_GETFD) != -1;
  return n;
}

static std::string base;  // a directory with a small tree in it (made by main)

static void make_tree(const std::string& root) {
  mkdir(root.c_str(), 0700);
  std::string d = root;
  for (const char* c : {"/first-level-directory-name", "/second-level-directory-name", "/third"}) {
    write_file(d + "/a-file-at-this-level-with-a-long-enough-name", "contents");
    d += c;
    mkdir(d.c_str(), 0700);
  }
}

// The operations: each one may throw or report an error.
static void op_ifstream() {
  std::ifstream in(base + "/tree/a-file-at-this-level-with-a-long-enough-name");
  std::string s;
  in >> s;
}
static void op_filebuf() {
  std::filebuf fb;
  if (fb.open(base + "/written-file-with-a-long-enough-name", std::ios::out | std::ios::trunc)) {
    fb.sputn("text", 4);
    fb.close();
  }
}
static void op_fstream_rw() {
  std::fstream f(base + "/read-write-file-with-a-long-enough-name", std::ios::in | std::ios::out | std::ios::trunc);
  f << "line one\nline two\n";
  f.seekg(0);
  std::string line;
  std::getline(f, line);
}
static void op_copy_file_ec() {
  std::error_code ec;
  fs::copy_file(base + "/tree/a-file-at-this-level-with-a-long-enough-name", base + "/copied-file-with-a-long-enough-name",
                fs::copy_options::overwrite_existing, ec);
}
static void op_copy_file() {
  fs::copy_file(base + "/tree/a-file-at-this-level-with-a-long-enough-name", base + "/copied-file-with-a-long-enough-name",
                fs::copy_options::overwrite_existing);
}
static void op_copy_tree() {
  std::error_code ec;
  fs::copy(base + "/tree", base + "/copied-tree-with-a-long-enough-name", fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
}
static void op_remove_all_ec() {
  make_tree(base + "/doomed-tree-with-a-long-enough-name");
  std::error_code ec;
  fs::remove_all(base + "/doomed-tree-with-a-long-enough-name", ec);
}
static void op_remove_all() {
  make_tree(base + "/doomed-tree-with-a-long-enough-name");
  fs::remove_all(base + "/doomed-tree-with-a-long-enough-name");
}
static void op_iterate() {
  long n = 0;
  for (auto it = fs::recursive_directory_iterator(base + "/tree"); it != fs::recursive_directory_iterator(); ++it) ++n;
  for (auto& e : fs::directory_iterator(base + "/tree")) n += e.is_directory();
  (void)n;
}
static void op_is_empty() {
  std::error_code ec;
  (void)fs::is_empty(base + "/tree", ec);
  (void)fs::is_empty(base + "/tree");
}
static void op_random_device() {
  std::random_device rd;
  (void)rd();
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
    {"ifstream", op_ifstream},           {"filebuf::open", op_filebuf},       {"fstream", op_fstream_rw},
    {"copy_file(ec)", op_copy_file_ec},  {"copy_file", op_copy_file},         {"copy recursive", op_copy_tree},
    {"remove_all(ec)", op_remove_all_ec}, {"remove_all", op_remove_all},      {"directory iterators", op_iterate},
    {"is_empty", op_is_empty},           {"random_device", op_random_device}, {"time zone database", op_tzdb},
    {"stacktrace", op_stacktrace},
};

static void attempt(void (*f)()) {
  try {
    f();
  } catch (...) {
  }
}

static long* shared_calls;  // (shared with the children) the calls made by the armed attempt

// In a child: the operation with failure k armed (0: none), then again with none. Exit status:
// bit 7 set when the failure was reached; the low bits the number of open descriptors.
static int child(void (*f)(), long k) {
  if (k == 0) {  // record the calls made while a descriptor is held
    const int d = fcntl(0, F_DUPFD, 0);
    close(d);
    probe_base = d;
  }
  fail_at = k;
  calls = 0;
  attempt(f);
  probe_base = -1;
  *shared_calls = calls;
  const bool reached = failed;
  fail_at = 0;
  attempt(f);
  const int n = open_fds();
  return (reached ? 128 : 0) | (n < 127 ? n : 127);
}

static int run_child(void (*f)(), long k) {
  const pid_t pid = fork();
  CHECK(pid >= 0);
  if (pid == 0) _exit(child(f, k));
  int status = 0;
  CHECK(waitpid(pid, &status, 0) == pid);
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

int main() {
  TmpDir tmp;
  base = tmp / "a-directory-name-longer-than-any-small-string-buffer";
  CHECK(mkdir(base.c_str(), 0700) == 0);
  make_tree(base + "/tree");
  shared_calls = static_cast<long*>(mmap(nullptr, sizeof(long), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
  CHECK(shared_calls != MAP_FAILED);
  holding = static_cast<unsigned char*>(mmap(nullptr, max_recorded, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
  CHECK(holding != MAP_FAILED);
  int failures = 0;
  int printed = 0;
  for (const Op& op : ops) {
    printed = 0;  // (at most three reports per operation)
    for (long i = 0; i < max_recorded; ++i) holding[i] = 0;
    const int expected = run_child(op.f, 0) & 127;
    const long total = *shared_calls;  // operator new calls of one unfailing attempt
    const long step = total <= 400 ? 1 : total / 200;
    auto held = [&](long k) { return k > 0 && k < max_recorded && holding[k]; };
    for (long k = 1; k <= total + 1; ++k) {
      if (!(k <= 200 || k % step == 0 || k == total + 1 || held(k - 1) || held(k) || held(k + 1))) continue;
      const int r = run_child(op.f, k);
      if (r < 0) {  // killed: std::terminate (an allocation failure escaping a noexcept function)
        ++failures;
        if (printed++ < 3) dprintf(2, "%s: operator new failing at call %ld: the process was killed\n", op.name, k);
        continue;
      }
      if ((r & 127) != expected) {
        ++failures;
        if (printed++ < 3)
          dprintf(2, "%s: operator new failing at call %ld leaves %d descriptor(s) open\n", op.name, k, (r & 127) - expected);
      }
      if (!(r & 128)) break;
    }
  }
  CHECK(failures == 0);
  return 0;
}
