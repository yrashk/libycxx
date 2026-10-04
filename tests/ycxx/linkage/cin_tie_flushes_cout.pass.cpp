// Whole-program: cin is tied to cout, so reading from cin flushes pending cout output before
// the program blocks for input, even with sync_with_stdio(false) (cout then has a buffer of its
// own).
//   [narrow.stream.objects]/2: "After the object cin is initialized, cin.tie() returns &cout."
//   [istream.sentry]/2: "if is.tie() is not a null pointer, the function calls is.tie()->flush()
//     ... Except that this call can be suppressed if the put area of is.tie() is empty. Further
//     an implementation is allowed to defer the call to flush until a call of
//     is.rdbuf()->underflow() occurs" - i.e. at the latest when cin needs more characters, which
//     is before it blocks waiting for them. Formatted (operator>>) and unformatted (getline)
//     input both construct a sentry ([istream.formatted.reqmts]/1, [istream.unformatted]/1).
//   [ios.members.static]: sync_with_stdio(false) before any I/O.
// The parent runs this program as a child with pipes for stdin and stdout, and writes each
// answer only after it has read the prompt; a prompt left in cout's buffer would deadlock (the
// parent gives up after 20 seconds).
#include <iostream>
#include <string>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "check.hpp"

static int child_main(bool unsync) {
  if (unsync) std::ios_base::sync_with_stdio(false);
  int x = 0;
  std::string name;
  std::cout << "number? ";  // no newline, no flush
  if (!(std::cin >> x)) return 2;
  std::cout << "got " << x << "; name? ";
  std::cin.ignore();
  if (!std::getline(std::cin, name)) return 3;
  std::cout << "hello " << name << "\n";
  return 0;
}

// Reads from fd until `out` ends with `want` (true) or 20 s pass / EOF (false).
static bool read_until(int fd, std::string& out, const std::string& want) {
  char buf[256];
  while (!out.ends_with(want)) {
    pollfd p{fd, POLLIN, 0};
    if (poll(&p, 1, 20000) <= 0) return false;
    ssize_t n = read(fd, buf, sizeof buf);
    if (n <= 0) return false;
    out.append(buf, static_cast<std::size_t>(n));
  }
  return true;
}

static void run(const char* mode) {
  int in[2], out[2];
  CHECK(pipe(in) == 0 && pipe(out) == 0);
  pid_t pid = fork();
  CHECK(pid >= 0);
  if (pid == 0) {
    dup2(in[0], 0);
    dup2(out[1], 1);
    close(in[0]); close(in[1]); close(out[0]); close(out[1]);
    char self[] = "/proc/self/exe";
    char m[32];
    strncpy(m, mode, sizeof m - 1);
    m[sizeof m - 1] = 0;
    char* argv[] = {self, m, nullptr};
    execv(self, argv);
    _exit(127);
  }
  close(in[0]);
  close(out[1]);
  std::string got;
  bool ok1 = read_until(out[0], got, "number? ");
  if (ok1) CHECK(write(in[1], "42\n", 3) == 3);
  bool ok2 = ok1 && read_until(out[0], got, "name? ");
  if (ok2) CHECK(write(in[1], "Ada Lovelace\n", 13) == 13);
  bool ok3 = ok2 && read_until(out[0], got, "\n");
  if (!ok3) kill(pid, SIGKILL);
  close(in[1]);
  int st = 0;
  waitpid(pid, &st, 0);
  close(out[0]);
  if (!ok3) dprintf(2, "%s: stalled; output so far: [%s]\n", mode, got.c_str());
  CHECK(ok1 && ok2 && ok3);
  CHECK(got == "number? got 42; name? hello Ada Lovelace\n");
  CHECK(WIFEXITED(st) && WEXITSTATUS(st) == 0);
}

int main(int argc, char** argv) {
  if (argc > 1) return child_main(strcmp(argv[1], "unsync") == 0);
  run("sync");
  run("unsync");
  return 0;
}
