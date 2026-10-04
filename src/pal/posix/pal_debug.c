// libycxx POSIX platform abstraction layer: the debugging hooks (<debugging>, <stacktrace>).
// Kept apart from pal.c, in C like it.
#define _GNU_SOURCE
#include <ycxx/pal.h>

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__linux__) || defined(__FreeBSD__)
#  include <link.h>
#endif

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

#if defined(__linux__) || defined(__FreeBSD__)
struct pal_object_query {
  ElfW(Addr) pc;
  char* path;
  size_t n;
  ycxx_pal_handle* bias;
  int found;
};

static int pal_object_callback(struct dl_phdr_info* info, size_t size, void* data) {
  (void)size;
  struct pal_object_query* q = (struct pal_object_query*)data;
  for (ElfW(Half) i = 0; i < info->dlpi_phnum; ++i) {
    const ElfW(Phdr)* ph = &info->dlpi_phdr[i];
    if (ph->p_type != PT_LOAD)
      continue;
    const ElfW(Addr) start = info->dlpi_addr + ph->p_vaddr;
    if (q->pc >= start && q->pc - start < ph->p_memsz) {
      // The executable has an empty name; the first object listed is the executable.
      const char* name = info->dlpi_name && *info->dlpi_name ? info->dlpi_name : "/proc/self/exe";
      size_t len = strlen(name);
      if (len >= q->n)
        len = q->n - 1;
      memcpy(q->path, name, len);
      q->path[len] = '\0';
      *q->bias = (ycxx_pal_handle)info->dlpi_addr;
      q->found = 1;
      return 1;
    }
  }
  return 0;
}
#endif

int ycxx_pal_object_of(ycxx_pal_handle pc, char* path, ycxx_pal_size n, ycxx_pal_handle* bias) {
  if (n == 0)
    return EINVAL;
#if defined(__linux__) || defined(__FreeBSD__)
  struct pal_object_query q = {(ElfW(Addr))pc, path, n, bias, 0};
  dl_iterate_phdr(pal_object_callback, &q);
  return q.found ? 0 : ENOENT;
#else
  (void)pc;
  (void)bias;
  path[0] = '\0';
  return ENOSYS;
#endif
}

int ycxx_pal_dynamic_symbol(ycxx_pal_handle pc, const char** name, ycxx_pal_handle* start) {
  Dl_info info;
  if (dladdr((const void*)pc, &info) == 0 || info.dli_sname == NULL)
    return ENOENT;
  *name = info.dli_sname;
  *start = (ycxx_pal_handle)info.dli_saddr;
  return 0;
}

int ycxx_pal_map_file(const char* path, const void** data, ycxx_pal_size* size) {
  const int saved = errno;
  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    int e = errno;
    errno = saved;
    return e;
  }
  struct stat st;
  int r = 0;
  if (fstat(fd, &st) != 0 || st.st_size <= 0) {
    r = EINVAL;
  } else {
    void* p = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (p == MAP_FAILED) {
      r = errno;
    } else {
      *data = p;
      *size = (ycxx_pal_size)st.st_size;
    }
  }
  close(fd);
  errno = saved;
  return r;
}

void ycxx_pal_unmap_file(const void* data, ycxx_pal_size size) { munmap((void*)data, size); }
