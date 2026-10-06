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
#if defined(__APPLE__)
#  include <sys/sysctl.h>
#  include <sys/types.h>
#endif

int __ycxx_pal_debugger_present(void) {
#if defined(__linux__)
  // "TracerPid:\t<pid>" in /proc/self/status: nonzero while a tracer (ptrace) is attached.
  const int __saved = errno;
  int __fd = open("/proc/self/status", O_RDONLY | O_CLOEXEC);
  if (__fd < 0) {
    errno = __saved;
    return 0;
  }
  char __buf[4096];
  size_t __len = 0;
  for (;;) {
    ssize_t r = read(__fd, __buf + __len, sizeof __buf - 1 - __len);
    if (r < 0 && errno == EINTR)
      continue;
    if (r <= 0)
      break;
    __len += (size_t)r;
    if (__len == sizeof __buf - 1)
      break;
  }
  close(__fd);
  errno = __saved;
  __buf[__len] = '\0';
  const char* p = strstr(__buf, "TracerPid:");
  if (!p)
    return 0;
  p += sizeof "TracerPid:" - 1;
  while (*p == ' ' || *p == '\t')
    ++p;
  return *p >= '1' && *p <= '9';
#elif defined(__APPLE__)
  // The P_TRACED flag of this process's kinfo_proc (Apple's Technical Q&A QA1361).
  int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, (int)getpid()};
  struct kinfo_proc info;
  memset(&info, 0, sizeof info);
  size_t size = sizeof info;
  const int __saved = errno;
  const int r = sysctl(mib, 4, &info, &size, NULL, 0);
  errno = __saved;
  return r == 0 && (info.kp_proc.p_flag & P_TRACED) != 0;
#else
  return 0;
#endif
}

#if defined(__linux__) || defined(__FreeBSD__)
struct pal_object_query {
  ElfW(Addr) __pc;
  char* path;
  size_t n;
  __ycxx_pal_handle* __bias;
  int found;
};

static int pal_object_callback(struct dl_phdr_info* info, size_t size, void* data) {
  (void)size;
  struct pal_object_query* __q = (struct pal_object_query*)data;
  for (ElfW(Half) i = 0; i < info->dlpi_phnum; ++i) {
    const ElfW(Phdr)* ph = &info->dlpi_phdr[i];
    if (ph->p_type != PT_LOAD)
      continue;
    const ElfW(Addr) start = info->dlpi_addr + ph->p_vaddr;
    if (__q->__pc >= start && __q->__pc - start < ph->p_memsz) {
      // The executable has an empty name; the first object listed is the executable.
      const char* name = info->dlpi_name && *info->dlpi_name ? info->dlpi_name : "/proc/self/exe";
      size_t __len = strlen(name);
      if (__len >= __q->n)
        __len = __q->n - 1;
      memcpy(__q->path, name, __len);
      __q->path[__len] = '\0';
      *__q->__bias = (__ycxx_pal_handle)info->dlpi_addr;
      __q->found = 1;
      return 1;
    }
  }
  return 0;
}
#endif

int __ycxx_pal_object_of(__ycxx_pal_handle __pc, char* path, __ycxx_pal_size n, __ycxx_pal_handle* __bias) {
  if (n == 0)
    return EINVAL;
#if defined(__linux__) || defined(__FreeBSD__)
  struct pal_object_query __q = {(ElfW(Addr))__pc, path, n, __bias, 0};
  dl_iterate_phdr(pal_object_callback, &__q);
  return __q.found ? 0 : ENOENT;
#else
  (void)__pc;
  (void)__bias;
  path[0] = '\0';
  return ENOSYS;
#endif
}

int __ycxx_pal_dynamic_symbol(__ycxx_pal_handle __pc, const char** name, __ycxx_pal_handle* start) {
  Dl_info info;
  if (dladdr((const void*)__pc, &info) == 0 || info.dli_sname == NULL)
    return ENOENT;
  *name = info.dli_sname;
  *start = (__ycxx_pal_handle)info.dli_saddr;
  return 0;
}

int __ycxx_pal_map_file(const char* path, const void** data, __ycxx_pal_size* size) {
  const int __saved = errno;
  int __fd = open(path, O_RDONLY | O_CLOEXEC);
  if (__fd < 0) {
    int e = errno;
    errno = __saved;
    return e;
  }
  struct stat __st;
  int r = 0;
  if (fstat(__fd, &__st) != 0 || __st.st_size <= 0) {
    r = EINVAL;
  } else {
    void* p = mmap(NULL, (size_t)__st.st_size, PROT_READ, MAP_PRIVATE, __fd, 0);
    if (p == MAP_FAILED) {
      r = errno;
    } else {
      *data = p;
      *size = (__ycxx_pal_size)__st.st_size;
    }
  }
  close(__fd);
  errno = __saved;
  return r;
}

void __ycxx_pal_unmap_file(const void* data, __ycxx_pal_size size) { munmap((void*)data, size); }
