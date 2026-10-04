// Whole-program tests: run this same executable again (/proc/self/exe) in a child process with
// argv[1] = mode and the environment variable YCXX_CHILD_MODE = mode, capturing its standard
// output and standard error separately. This lets a test check what a program writes after main
// returns (static destructors, atexit functions, stream flushing at exit), which the program
// itself cannot observe. Only POSIX calls are used, so the capture does not depend on the
// library under test.
#pragma once

#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>

struct ChildResult {
  int status = -1;  // exit status if exited normally, else -1 (killed by a signal: 1000 + signal)
  std::string out, err;
};

// The mode this process runs in: nullptr in the parent.
inline const char* child_mode() { return getenv("YCXX_CHILD_MODE"); }
inline bool child_mode_is(const char* m) {
  const char* c = child_mode();
  return c && strcmp(c, m) == 0;
}

inline ChildResult run_self(const char* mode) {
  int o[2], e[2];
  if (pipe(o) != 0 || pipe(e) != 0) abort();
  pid_t pid = fork();
  if (pid < 0) abort();
  if (pid == 0) {
    dup2(o[1], 1);
    dup2(e[1], 2);
    close(o[0]); close(o[1]); close(e[0]); close(e[1]);
    setenv("YCXX_CHILD_MODE", mode, 1);
    char self[] = "/proc/self/exe";
    char m[256];
    strncpy(m, mode, sizeof m - 1);
    m[sizeof m - 1] = 0;
    char* argv[] = {self, m, nullptr};
    execv(self, argv);
    _exit(127);
  }
  close(o[1]);
  close(e[1]);
  ChildResult r;
  pollfd fds[2] = {{o[0], POLLIN, 0}, {e[0], POLLIN, 0}};
  int open_fds = 2;
  char buf[4096];
  while (open_fds > 0) {
    if (poll(fds, 2, 60000) <= 0) break;
    for (int i = 0; i < 2; ++i) {
      if (fds[i].fd < 0 || !(fds[i].revents & (POLLIN | POLLHUP | POLLERR))) continue;
      ssize_t n = read(fds[i].fd, buf, sizeof buf);
      if (n <= 0) {
        close(fds[i].fd);
        fds[i].fd = -1;
        --open_fds;
      } else {
        (i == 0 ? r.out : r.err).append(buf, static_cast<std::size_t>(n));
      }
    }
  }
  int st = 0;
  waitpid(pid, &st, 0);
  r.status = WIFEXITED(st) ? WEXITSTATUS(st) : WIFSIGNALED(st) ? 1000 + WTERMSIG(st) : -1;
  return r;
}

// Prints a mismatch to the (real) standard error of the parent.
inline bool same_text(const std::string& got, const std::string& want, const char* what) {
  if (got == want) return true;
  dprintf(2, "%s mismatch\n--- got ---\n%s\n--- want ---\n%s\n--- end ---\n", what, got.c_str(), want.c_str());
  return false;
}
