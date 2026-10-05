// [iostream.objects.overview]/7: "Concurrent access to a synchronized ([ios.members.static])
// standard iostream object's formatted and unformatted input ([istream]) and output
// ([ostream]) functions or a standard C stream by multiple threads does not result in a data
// race"; Note 2: characters of different threads may interleave. [ios.members.static]: by
// default the standard streams are synchronized with the C streams, so a thread's output
// through cout and through printf/fputs on stdout (and through cerr/clog and stderr) appears
// in that thread's program order.
// A child process writes from main (before any thread exists, then after the threads are
// joined) and from four threads at once, each thread with its own alphabet, through formatted
// (const char*, char, int via num_put) and unformatted (put, write) output and the C stdio
// functions; the parent captures stdout and stderr and checks that every thread's characters,
// filtered out of the captured text, are exactly what that thread wrote, in order, and that
// nothing was lost or duplicated.
// FLAGS: -pthread
#include <cstdio>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "child_process.hpp"
#include "check.hpp"

constexpr int K = 4;
constexpr int R = 3000;
static const char* const alphabet[K] = {"ABCDEFG1", "HIJKLMN2", "OPQRSTU3", "abcdefg4"};

// what thread k writes per round, as one string
static std::string round_text(int k) {
  const char* a = alphabet[k];
  std::string s;
  s += a[0];
  s += a[1];  // cout << "AB"
  s += a[2];  // put
  s += a[3];
  s += a[4];  // write
  s += a[5];  // fputs / printf
  s += std::string(3, a[7]);  // cout << 111 * (k + 1)
  s += a[6];  // cout << char
  return s;
}

static void writer(int k, std::ostream& os, std::FILE* c) {
  const char* a = alphabet[k];
  const char two[3] = {a[0], a[1], 0};
  const int num = 111 * (k + 1);
  for (int r = 0; r < R; ++r) {
    os << two;
    os.put(a[2]);
    os.write(a + 3, 2);
    if (r % 2) std::fputc(a[5], c);
    else std::fprintf(c, "%c", a[5]);
    os << num << a[6];
  }
}

static int child(const char* mode) {
  const bool err = std::string(mode) == "err";
  std::ostream& os = err ? std::cerr : std::cout;
  std::FILE* c = err ? stderr : stdout;
  os << "!!";
  std::fputs("!", c);
  std::vector<std::thread> ts;
  for (int k = 0; k < K; ++k) {
    if (err && k == 1) {
      ts.emplace_back(writer, k, std::ref(std::clog), c);  // clog also goes to stderr
    } else {
      ts.emplace_back(writer, k, std::ref(os), c);
    }
  }
  for (auto& t : ts) t.join();
  os << "!!" << std::flush;
  std::clog << std::flush;
  return 0;
}

static void verify(const std::string& out) {
  for (int k = 0; k < K; ++k) {
    std::string want;
    const std::string one = round_text(k);
    for (int r = 0; r < R; ++r) want += one;
    std::string got;
    const std::string alpha = alphabet[k];
    for (char ch : out)
      if (alpha.find(ch) != std::string::npos) got += ch;
    if (got != want) {
      dprintf(2, "thread %d: %zu characters, want %zu\n", k, got.size(), want.size());
      std::size_t i = 0;
      while (i < got.size() && i < want.size() && got[i] == want[i]) ++i;
      dprintf(2, "first difference at %zu\n", i);
    }
    CHECK(got == want);
  }
  std::size_t bangs = 0, others = 0;
  for (char ch : out) {
    if (ch == '!') ++bangs;
    else {
      bool any = false;
      for (int k = 0; k < K; ++k)
        if (std::string(alphabet[k]).find(ch) != std::string::npos) any = true;
      if (!any) ++others;
    }
  }
  CHECK(bangs == 5);
  CHECK(others == 0);
  CHECK(out.substr(0, 3) == "!!!");
  CHECK(out.substr(out.size() - 2) == "!!");
}

int main(int argc, char** argv) {
  if (child_mode()) return child(argv[1]);
  (void)argc;
  ChildResult o = run_self("out");
  CHECK(o.status == 0);
  CHECK(o.err.empty());
  verify(o.out);
  ChildResult e = run_self("err");
  CHECK(e.status == 0);
  CHECK(e.out.empty());
  verify(e.err);
}
