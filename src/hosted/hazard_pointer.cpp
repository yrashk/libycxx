// libycxx hosted runtime: the hazard pointer records and the retired list
// (ycxx/hosted/hazard_pointer.hpp).
#include <new>
#include <ycxx/hosted/hazard_pointer.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
namespace {

__hp_record* records = nullptr;        // atomic; push-only
__ycxx_pal_u32 record_count = 0;       // atomic
__hp_retired_node* retired = nullptr;  // atomic; the retired objects not yet reclaimed
__ycxx_pal_u32 retired_count = 0;      // atomic; about the length of `retired`

// Reclamation starts when this many objects are retired: proportional to the number of hazard
// pointers, so each scan reclaims at least about half of what it looks at.
__ycxx_pal_u32 reclaim_threshold() noexcept { return 2 * __atomic_load_n(&record_count, __ATOMIC_RELAXED) + 64; }

void push_retired(__hp_retired_node* first, __hp_retired_node* last) noexcept {
  __hp_retired_node* __head = __atomic_load_n(&retired, __ATOMIC_RELAXED);
  do
    last->__hp_next_ = __head;
  while (!__atomic_compare_exchange_n(&retired, &__head, first, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED));
}

bool is_protected(const void* __object) noexcept {
  for (__hp_record* r = __atomic_load_n(&records, __ATOMIC_ACQUIRE); r; r = r->next)
    if (__atomic_load_n(&r->value, __ATOMIC_SEQ_CST) == __object)
      return true;
  return false;
}

// Takes the retired list, reclaims every object no hazard pointer is associated with, and puts
// the others back.
void __reclaim() noexcept {
  __hp_retired_node* list = __atomic_exchange_n(&retired, static_cast<__hp_retired_node*>(nullptr), __ATOMIC_ACQUIRE);
  if (!list)
    return;
  // Pairs with the fence after a hazard pointer is set (hazard_pointer::set): an object whose
  // replacement the reader's re-read missed is seen protected here.
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  __hp_retired_node* keep_first = nullptr;
  __hp_retired_node* keep_last = nullptr;
  __ycxx_pal_u32 __kept = 0, taken = 0;
  while (list) {
    __hp_retired_node* n = list;
    list = n->__hp_next_;
    ++taken;
    if (is_protected(n->__hp_object_)) {
      n->__hp_next_ = keep_first;
      keep_first = n;
      if (!keep_last)
        keep_last = n;
      ++__kept;
    } else {
      n->__hp_reclaim_(n);
    }
  }
  __atomic_fetch_sub(&retired_count, taken - __kept, __ATOMIC_RELAXED);
  if (keep_first)
    push_retired(keep_first, keep_last);
}

} // namespace

__hp_record* __hp_acquire() {
  for (__hp_record* r = __atomic_load_n(&records, __ATOMIC_ACQUIRE); r; r = r->next) {
    __ycxx_pal_u32 free = 0;
    if (__atomic_load_n(&r->__owned, __ATOMIC_RELAXED) == 0 &&
        __atomic_compare_exchange_n(&r->__owned, &free, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
      return r;
  }
  __hp_record* r = new __hp_record{nullptr, nullptr, 1};
  __hp_record* __head = __atomic_load_n(&records, __ATOMIC_RELAXED);
  do
    r->next = __head;
  while (!__atomic_compare_exchange_n(&records, &__head, r, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED));
  __atomic_fetch_add(&record_count, 1, __ATOMIC_RELAXED);
  return r;
}

void __hp_release(__hp_record* r) noexcept {
  __atomic_store_n(&r->value, static_cast<const void*>(nullptr), __ATOMIC_RELEASE);
  __atomic_store_n(&r->__owned, 0, __ATOMIC_RELEASE);
}

void __hp_retire(__hp_retired_node* n) noexcept {
  push_retired(n, n);
  if (__atomic_add_fetch(&retired_count, 1, __ATOMIC_RELAXED) >= reclaim_threshold())
    __reclaim();
}

}} // namespace __ycxx::__detail
