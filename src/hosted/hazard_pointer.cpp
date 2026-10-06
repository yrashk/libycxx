// libycxx hosted runtime: the hazard pointer records and the retired list
// (ycxx/hosted/hazard_pointer.hpp).
#include <new>
#include <ycxx/hosted/hazard_pointer.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
namespace {

hp_record* records = nullptr;        // atomic; push-only
ycxx_pal_u32 record_count = 0;       // atomic
hp_retired_node* retired = nullptr;  // atomic; the retired objects not yet reclaimed
ycxx_pal_u32 retired_count = 0;      // atomic; about the length of `retired`

// Reclamation starts when this many objects are retired: proportional to the number of hazard
// pointers, so each scan reclaims at least about half of what it looks at.
ycxx_pal_u32 reclaim_threshold() noexcept { return 2 * __atomic_load_n(&record_count, __ATOMIC_RELAXED) + 64; }

void push_retired(hp_retired_node* first, hp_retired_node* last) noexcept {
  hp_retired_node* head = __atomic_load_n(&retired, __ATOMIC_RELAXED);
  do
    last->hp_next_ = head;
  while (!__atomic_compare_exchange_n(&retired, &head, first, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED));
}

bool is_protected(const void* object) noexcept {
  for (hp_record* r = __atomic_load_n(&records, __ATOMIC_ACQUIRE); r; r = r->next)
    if (__atomic_load_n(&r->value, __ATOMIC_SEQ_CST) == object)
      return true;
  return false;
}

// Takes the retired list, reclaims every object no hazard pointer is associated with, and puts
// the others back.
void reclaim() noexcept {
  hp_retired_node* list = __atomic_exchange_n(&retired, static_cast<hp_retired_node*>(nullptr), __ATOMIC_ACQUIRE);
  if (!list)
    return;
  // Pairs with the fence after a hazard pointer is set (hazard_pointer::set): an object whose
  // replacement the reader's re-read missed is seen protected here.
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  hp_retired_node* keep_first = nullptr;
  hp_retired_node* keep_last = nullptr;
  ycxx_pal_u32 kept = 0, taken = 0;
  while (list) {
    hp_retired_node* n = list;
    list = n->hp_next_;
    ++taken;
    if (is_protected(n->hp_object_)) {
      n->hp_next_ = keep_first;
      keep_first = n;
      if (!keep_last)
        keep_last = n;
      ++kept;
    } else {
      n->hp_reclaim_(n);
    }
  }
  __atomic_fetch_sub(&retired_count, taken - kept, __ATOMIC_RELAXED);
  if (keep_first)
    push_retired(keep_first, keep_last);
}

} // namespace

hp_record* hp_acquire() {
  for (hp_record* r = __atomic_load_n(&records, __ATOMIC_ACQUIRE); r; r = r->next) {
    ycxx_pal_u32 free = 0;
    if (__atomic_load_n(&r->owned, __ATOMIC_RELAXED) == 0 &&
        __atomic_compare_exchange_n(&r->owned, &free, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
      return r;
  }
  hp_record* r = new hp_record{nullptr, nullptr, 1};
  hp_record* head = __atomic_load_n(&records, __ATOMIC_RELAXED);
  do
    r->next = head;
  while (!__atomic_compare_exchange_n(&records, &head, r, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED));
  __atomic_fetch_add(&record_count, 1, __ATOMIC_RELAXED);
  return r;
}

void hp_release(hp_record* r) noexcept {
  __atomic_store_n(&r->value, static_cast<const void*>(nullptr), __ATOMIC_RELEASE);
  __atomic_store_n(&r->owned, 0, __ATOMIC_RELEASE);
}

void hp_retire(hp_retired_node* n) noexcept {
  push_retired(n, n);
  if (__atomic_add_fetch(&retired_count, 1, __ATOMIC_RELAXED) >= reclaim_threshold())
    reclaim();
}

}} // namespace ycxx::detail
