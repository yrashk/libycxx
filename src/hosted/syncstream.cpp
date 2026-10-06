// libycxx hosted runtime: the per-stream-buffer locks of basic_syncbuf::emit()
// ([syncstream.syncbuf.members]/5: "a lock uniquely associated with wrapped").
//
// A lock exists only while some emit() holds or waits for it: a slot naming the stream buffer it
// locks and counting its users, guarded by one table lock that is held only to find or release a
// slot (never while a buffer is written). Two buffers never share a lock, so an emit to a syncbuf
// that emits onward to another buffer cannot deadlock, nor can emits to distinct buffers wait for
// each other. The slots are a fixed table, then, when every one is in use, slots allocated from
// the platform layer and freed with their last user; only if that allocation fails does a new
// buffer's emit wait for a slot to be released.
#include <ycxx/hosted/memory_resource.hpp>
#include <ycxx/pal.h>

namespace {

struct __slot {
  const void* key = nullptr; // null: free (table slots)
  std::uint32_t users = 0;   // emits holding or waiting for `lock`
  __ycxx::__detail::__pal_lock lock;
  __slot* next = nullptr; // the allocated slots in use, a list
};

constexpr unsigned slot_count = 64;
constinit __slot __slots[slot_count];
constinit __slot* __extra = nullptr; // allocated slots in use
constinit __ycxx::__detail::__pal_lock table_lock;
constinit std::uint32_t releases = 0; // bumped when a slot is freed; waited on when none is free

bool allocated(const __slot* s) noexcept {
  const auto a = reinterpret_cast<__UINTPTR_TYPE__>(s), t = reinterpret_cast<__UINTPTR_TYPE__>(__slots);
  return a - t >= sizeof __slots; // not one of the table's (also when a < t)
}

// Under table_lock: the slot for key, a new one for it, or null when none can be had.
__slot* find_slot(const void* key) noexcept {
  __slot* free_slot = nullptr;
  for (__slot& candidate : __slots) {
    if (candidate.key == key)
      return &candidate;
    if (candidate.key == nullptr && free_slot == nullptr)
      free_slot = &candidate;
  }
  for (__slot* s = __extra; s != nullptr; s = s->next)
    if (s->key == key)
      return s;
  if (free_slot == nullptr) {
    void* __mem = ycxx_pal_allocate(sizeof(__slot), alignof(__slot));
    if (__mem == nullptr)
      return nullptr;
    free_slot = ::new (__mem) __slot;
    free_slot->next = __extra;
    __extra = free_slot;
  }
  free_slot->key = key;
  return free_slot;
}

} // namespace

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

void* __syncbuf_lock(const void* key) noexcept {
  __slot* s;
  for (;;) {
    table_lock.lock();
    s = find_slot(key);
    if (s != nullptr) {
      ++s->users;
      table_lock.unlock();
      break;
    }
    const std::uint32_t __seen = __atomic_load_n(&releases, __ATOMIC_RELAXED);
    table_lock.unlock();
    ycxx_pal_wait(&releases, __seen);
  }
  s->lock.lock();
  return s;
}

void __syncbuf_unlock(void* handle) noexcept {
  __slot* s = static_cast<__slot*>(handle);
  s->lock.unlock();
  table_lock.lock();
  const bool freed = --s->users == 0;
  if (freed) {
    s->key = nullptr;
    if (allocated(s)) {
      __slot** link = &__extra;
      while (*link != s)
        link = &(*link)->next;
      *link = s->next;
    }
  }
  table_lock.unlock();
  if (freed) {
    if (allocated(s))
      ycxx_pal_deallocate(s, sizeof(__slot), alignof(__slot));
    __atomic_fetch_add(&releases, 1, __ATOMIC_RELAXED);
    ycxx_pal_wake_all(&releases);
  }
}

}} // namespace __ycxx::__detail
