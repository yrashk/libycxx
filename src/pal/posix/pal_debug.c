// libycxx POSIX platform abstraction layer: the debugging hooks (<debugging>, <stacktrace>).
// Kept apart from pal.c, in C like it.
#define _GNU_SOURCE
#include <ycxx/pal.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int ycxx_pal_debugger_present(void) {
#if defined(__linux__)
  // "TracerPid:\t<pid>" in /proc/self/status: nonzero while a tracer (ptrace) is attached.
  const int saved = errno;
  int fd = open("/proc/self/status", O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    errno = saved;
    return 0;
  }
  char buf[4096];
  size_t len = 0;
  for (;;) {
    ssize_t r = read(fd, buf + len, sizeof buf - 1 - len);
    if (r < 0 && errno == EINTR)
      continue;
    if (r <= 0)
      break;
    len += (size_t)r;
    if (len == sizeof buf - 1)
      break;
  }
  close(fd);
  errno = saved;
  buf[len] = '\0';
  const char* p = strstr(buf, "TracerPid:");
  if (!p)
    return 0;
  p += sizeof "TracerPid:" - 1;
  while (*p == ' ' || *p == '\t')
    ++p;
  return *p >= '1' && *p <= '9';
#else
  return 0;
#endif
}
