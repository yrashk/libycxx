// libycxx hosted runtime: the per-stream-buffer locks of basic_syncbuf::emit()
// ([syncstream.syncbuf.members]/5: "a lock uniquely associated with wrapped").
//
// A lock exists only while some emit() holds or waits for it: a fixed table of slots, each
// naming the stream buffer it locks and counting its users, guarded by one table lock that is
// held only to find or release a slot (never while a buffer is written). Two buffers never share
// a lock, so an emit to a syncbuf that emits onward to another buffer cannot deadlock. If every
// slot is in use, a new buffer's emit waits for a slot to be released.
#include <ycxx/hosted/memory_resource.hpp>
#include <ycxx/pal.h>

namespace {

struct slot {
  const void* key = nullptr; // null: free
  std::uint32_t users = 0;   // emits holding or waiting for `lock`
  ycxx::detail::pal_lock lock;
};

constexpr unsigned slot_count = 64;
constinit slot slots[slot_count];
constinit ycxx::detail::pal_lock table_lock;
constinit std::uint32_t releases = 0; // bumped when a slot is freed; waited on when all are taken

} // namespace

namespace ycxx::detail {

void* syncbuf_lock(const void* key) noexcept {
  slot* s = nullptr;
  for (;;) {
    table_lock.lock();
    slot* free_slot = nullptr;
    for (slot& candidate : slots) {
      if (candidate.key == key) {
        s = &candidate;
        break;
      }
      if (candidate.key == nullptr && free_slot == nullptr)
        free_slot = &candidate;
    }
    if (s == nullptr && free_slot != nullptr) {
      s = free_slot;
      s->key = key;
    }
    if (s != nullptr) {
      ++s->users;
      table_lock.unlock();
      break;
    }
    const std::uint32_t seen = __atomic_load_n(&releases, __ATOMIC_RELAXED);
    table_lock.unlock();
    ycxx_pal_wait(&releases, seen);
  }
  s->lock.lock();
  return s;
}

void syncbuf_unlock(void* handle) noexcept {
  slot* s = static_cast<slot*>(handle);
  s->lock.unlock();
  table_lock.lock();
  const bool freed = --s->users == 0;
  if (freed)
    s->key = nullptr;
  table_lock.unlock();
  if (freed) {
    __atomic_fetch_add(&releases, 1, __ATOMIC_RELAXED);
    ycxx_pal_wake_all(&releases);
  }
}

} // namespace ycxx::detail
