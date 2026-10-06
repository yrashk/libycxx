/* A 'files' hosted layer of the program's own: the file streams of libycxx over a RAM disk (a
 * fixed table of named files whose contents live on the 'memory' layer's heap), as a program
 * would put them over its own block device (examples/hosted-layers/README.md). The six
 * ycxx_pal_file_* primitives of ycxx/pal.h. Plain C; one thread of execution. */
#include <ycxx/pal.h>

#include "../common/heap.h"

#include <errno.h>
#include <string.h>

enum { max_files = 16, max_open = 16, max_name = 64 };

struct ram_file {
  char name[max_name]; /* empty: the slot is free */
  unsigned char* data;
  ycxx_pal_size size, capacity;
};
struct open_file {
  struct ram_file* file; /* null: the slot is free */
  ycxx_pal_size pos;
  int flags;
};
static struct ram_file files[max_files];
static struct open_file opened[max_open];

static struct ram_file* find(const char* name) {
  for (int i = 0; i < max_files; ++i)
    if (files[i].name[0] != '\0' && strcmp(files[i].name, name) == 0)
      return &files[i];
  return 0;
}

static struct open_file* of(ycxx_pal_handle h) {
  return h >= 1 && h <= max_open && opened[h - 1].file != 0 ? &opened[h - 1] : 0;
}

int ycxx_pal_file_open(const char* name, int flags, ycxx_pal_handle* h) {
  if (strlen(name) >= max_name)
    return ENAMETOOLONG;
  struct ram_file* f = find(name);
  if (f != 0 && (flags & ycxx_pal_file_create) && (flags & ycxx_pal_file_exclusive))
    return EEXIST;
  if (f == 0) {
    if (!(flags & ycxx_pal_file_create))
      return ENOENT;
    for (int i = 0; i < max_files && f == 0; ++i)
      if (files[i].name[0] == '\0')
        f = &files[i];
    if (f == 0)
      return ENOSPC;
    strcpy(f->name, name);
    f->data = 0;
    f->size = f->capacity = 0;
  }
  for (int i = 0; i < max_open; ++i) {
    if (opened[i].file == 0) {
      if (flags & ycxx_pal_file_truncate)
        f->size = 0;
      opened[i].file = f;
      opened[i].pos = 0;
      opened[i].flags = flags;
      *h = (ycxx_pal_handle)(i + 1);
      return 0;
    }
  }
  return EMFILE;
}

int ycxx_pal_file_close(ycxx_pal_handle h) {
  struct open_file* o = of(h);
  if (o == 0)
    return EBADF;
  o->file = 0;
  return 0;
}

int ycxx_pal_file_read(ycxx_pal_handle h, void* data, ycxx_pal_size n, ycxx_pal_size* got) {
  struct open_file* o = of(h);
  *got = 0;
  if (o == 0 || !(o->flags & ycxx_pal_file_read_access))
    return EBADF;
  if (o->pos < o->file->size) {
    const ycxx_pal_size left = o->file->size - o->pos;
    *got = n < left ? n : left;
    memcpy(data, o->file->data + o->pos, *got);
    o->pos += *got;
  }
  return 0;
}

int ycxx_pal_file_write(ycxx_pal_handle h, const void* data, ycxx_pal_size n, ycxx_pal_size* written) {
  struct open_file* o = of(h);
  *written = 0;
  if (o == 0 || !(o->flags & ycxx_pal_file_write_access))
    return EBADF;
  struct ram_file* f = o->file;
  if (o->flags & ycxx_pal_file_append)
    o->pos = f->size;
  const ycxx_pal_size end = o->pos + n;
  if (end > f->capacity) { /* grow on the heap (the 'memory' layer's) */
    ycxx_pal_size cap = f->capacity ? f->capacity : 256;
    while (cap < end)
      cap *= 2;
    unsigned char* d = heap_allocate(cap, 16);
    if (d == 0)
      return ENOSPC;
    if (f->size != 0)
      memcpy(d, f->data, f->size);
    heap_free(f->data);
    f->data = d;
    f->capacity = cap;
  }
  if (o->pos > f->size) /* a write after a seek past the end: the gap reads as zeros */
    memset(f->data + f->size, 0, o->pos - f->size);
  memcpy(f->data + o->pos, data, n);
  o->pos = end;
  if (end > f->size)
    f->size = end;
  *written = n;
  return 0;
}

int ycxx_pal_file_seek(ycxx_pal_handle h, ycxx_pal_i64 off, int whence, ycxx_pal_i64* pos) {
  struct open_file* o = of(h);
  if (o == 0)
    return EBADF;
  const ycxx_pal_i64 base = whence == 0 ? 0 : whence == 1 ? (ycxx_pal_i64)o->pos : (ycxx_pal_i64)o->file->size;
  if (whence < 0 || whence > 2 || base + off < 0)
    return EINVAL;
  o->pos = (ycxx_pal_size)(base + off);
  *pos = base + off;
  return 0;
}

int ycxx_pal_file_flush(ycxx_pal_handle h) { return of(h) != 0 ? 0 : EBADF; }

/* For the program: the size of a file on the RAM disk, or -1. */
long ramdisk_file_size(const char* name) {
  const struct ram_file* f = find(name);
  return f != 0 ? (long)f->size : -1;
}
