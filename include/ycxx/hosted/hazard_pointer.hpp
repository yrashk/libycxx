// libycxx hosted: hazard pointers ([saferecl.hp]).
//
// The hazard pointers of the program are records in one push-only list of the hosted runtime
// (src/hosted/hazard_pointer.cpp); a record is never freed, and is reused once its owner is
// destroyed. A retired object is linked into the runtime's retired list through the node in its
// hazard_pointer_obj_base (retiring allocates nothing). When the list has grown past a bound
// proportional to the number of records, the retiring thread takes the whole list, reads every
// hazard pointer and reclaims each object no hazard pointer is associated with; the others go
// back to the list.
//
// Ordering: setting a hazard pointer is a store followed by a sequentially consistent fence
// before the re-read of the source, and a reclaimer issues a sequentially consistent fence
// between taking the retired list and reading the hazard pointers, so either the reader's re-read
// sees the replacement that preceded the retirement, or the reclaimer sees the hazard pointer.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/atomic.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/span.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/unique_ptr.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {
// The link of a retired object (the base of every hazard_pointer_obj_base).
struct __hp_retired_node {
  __hp_retired_node* __hp_next_;
  const void* __hp_object_;                          // the T object the hazard pointers name
  void (*__hp_reclaim_)(__hp_retired_node*) noexcept; // invokes the deleter
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
using __adl_free::__hp_retired_node;

// A hazard pointer: the value its owner publishes, and whether a hazard_pointer owns it.
struct __hp_record {
  const void* value;  // atomic
  __hp_record* next;    // immutable once the record is in the list
  ycxx_pal_u32 __owned; // atomic
};

// ---- the hosted runtime (src/hosted/hazard_pointer.cpp) ---------------------------------------
// An unowned record, now owned by the caller; throws bad_alloc if a new one cannot be allocated.
__hp_record* __hp_acquire();
void __hp_release(__hp_record* r) noexcept;
// Retires n (its object and reclaim function are set); may reclaim retired objects.
void __hp_retire(__hp_retired_node* n) noexcept;

// [saferecl.hp.general]/2: exactly one public, non-virtual base hazard_pointer_obj_base<T, D>.
template <class _Tp, class _Base>
concept __hp_protectable_via = std::is_base_of_v<_Base, _Tp> && requires(_Tp* p) { static_cast<_Base*>(p); } &&
                             requires(_Base* b) { static_cast<_Tp*>(b); };

template <class _Tp>
concept __hazard_protectable = requires { typename _Tp::__ycxx_hp_base; } &&
                             std::is_same_v<typename _Tp::__ycxx_hp_base::__ycxx_hp_object, std::remove_cv_t<_Tp>> &&
                             __hp_protectable_via<std::remove_cv_t<_Tp>, typename _Tp::__ycxx_hp_base>;

struct __hp_access;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [saferecl.hp.base]
template <class _Tp, class _Dp = default_delete<_Tp>>
class hazard_pointer_obj_base : __ycxx::__adl_free::__hp_retired_node {
  [[no_unique_address]] _Dp __deleter_;

  static void __reclaim(__ycxx::__adl_free::__hp_retired_node* n) noexcept {
    hazard_pointer_obj_base* __self = static_cast<hazard_pointer_obj_base*>(n);
    __self->__deleter_(static_cast<_Tp*>(__self));
  }

public:
  // For the hazard-protectable check: the base class of T (ambiguous, so absent, when T has
  // several hazard_pointer_obj_base bases).
  using __ycxx_hp_base = hazard_pointer_obj_base;
  using __ycxx_hp_object = _Tp;

  void retire(_Dp d = _Dp()) noexcept {
    static_assert(__ycxx::__detail::__hazard_protectable<_Tp>, "hazard_pointer_obj_base::retire: T is not hazard-protectable");
    __deleter_ = static_cast<_Dp&&>(d);
    _Tp* __x = static_cast<_Tp*>(this);
    this->__hp_object_ = static_cast<const void*>(__x);
    this->__hp_reclaim_ = &__reclaim;
    __ycxx::__detail::__hp_retire(this);
  }

protected:
  hazard_pointer_obj_base() = default;
  hazard_pointer_obj_base(const hazard_pointer_obj_base&) = default;
  hazard_pointer_obj_base(hazard_pointer_obj_base&&) = default;
  hazard_pointer_obj_base& operator=(const hazard_pointer_obj_base&) = default;
  hazard_pointer_obj_base& operator=(hazard_pointer_obj_base&&) = default;
  ~hazard_pointer_obj_base() = default;
};

// [saferecl.hp.holder]
class hazard_pointer {
  __ycxx::__detail::__hp_record* __rec_ = nullptr;

  explicit hazard_pointer(__ycxx::__detail::__hp_record* r) noexcept : __rec_(r) {}
  friend struct __ycxx::__detail::__hp_access;

  void set(const void* p) noexcept {
    __ycxx::__detail::__precondition(__rec_ != nullptr, "hazard_pointer: *this is empty");
    __atomic_store_n(&__rec_->value, p, __ATOMIC_SEQ_CST);
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
  }

public:
  hazard_pointer() noexcept = default;
  hazard_pointer(hazard_pointer&& other) noexcept : __rec_(other.__rec_) { other.__rec_ = nullptr; }
  hazard_pointer& operator=(hazard_pointer&& other) noexcept {
    if (this != __builtin_addressof(other)) {
      if (__rec_)
        __ycxx::__detail::__hp_release(__rec_);
      __rec_ = other.__rec_;
      other.__rec_ = nullptr;
    }
    return *this;
  }
  ~hazard_pointer() {
    if (__rec_)
      __ycxx::__detail::__hp_release(__rec_);
  }

  [[nodiscard]] bool empty() const noexcept { return __rec_ == nullptr; }

  template <class _Tp>
  _Tp* protect(const atomic<_Tp*>& __src) noexcept {
    _Tp* ptr = __src.load(memory_order::relaxed);
    while (!try_protect(ptr, __src)) {
    }
    return ptr;
  }
  template <class _Tp>
  bool try_protect(_Tp*& ptr, const atomic<_Tp*>& __src) noexcept {
    static_assert(__ycxx::__detail::__hazard_protectable<_Tp>, "hazard_pointer::try_protect: T is not hazard-protectable");
    _Tp* const __old = ptr;
    reset_protection(__old);
    ptr = __src.load(memory_order::acquire);
    if (__old != ptr) {
      reset_protection();
      return false;
    }
    return true;
  }
  template <class _Tp>
  void reset_protection(const _Tp* ptr) noexcept {
    static_assert(__ycxx::__detail::__hazard_protectable<_Tp>,
                  "hazard_pointer::reset_protection: T is not hazard-protectable");
    set(static_cast<const void*>(ptr));
  }
  void reset_protection(nullptr_t = nullptr) noexcept {
    __ycxx::__detail::__precondition(__rec_ != nullptr, "hazard_pointer: *this is empty");
    __atomic_store_n(&__rec_->value, static_cast<const void*>(nullptr), __ATOMIC_RELEASE);
  }
  void swap(hazard_pointer& other) noexcept {
    __ycxx::__detail::__hp_record* r = __rec_;
    __rec_ = other.__rec_;
    other.__rec_ = r;
  }
};

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
struct __hp_access {
  static std::hazard_pointer __make(__hp_record* r) noexcept { return std::hazard_pointer(r); }
  static __hp_record*& __record(std::hazard_pointer& h) noexcept { return h.__rec_; }
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [saferecl.hp.holder.nonmem]
[[nodiscard]] inline hazard_pointer make_hazard_pointer() {
  return __ycxx::__detail::__hp_access::__make(__ycxx::__detail::__hp_acquire());
}
inline void swap(hazard_pointer& a, hazard_pointer& b) noexcept { a.swap(b); }

inline void make_hazard_pointer_batch(span<hazard_pointer> __batch) {
  // All or nothing: a record this call gave an element is marked (owned == 2) until every
  // element has one; if an acquisition throws, the marked ones are given back.
  struct __undo {
    span<hazard_pointer> b;
    ~__undo() {
      for (hazard_pointer& h : b) {
        __ycxx::__detail::__hp_record*& r = __ycxx::__detail::__hp_access::__record(h);
        if (r && __atomic_load_n(&r->__owned, __ATOMIC_RELAXED) == 2) {
          __ycxx::__detail::__hp_release(r);
          r = nullptr;
        }
      }
    }
  } __u{__batch};
  for (hazard_pointer& h : __batch) {
    __ycxx::__detail::__hp_record*& r = __ycxx::__detail::__hp_access::__record(h);
    if (!r) {
      r = __ycxx::__detail::__hp_acquire();
      __atomic_store_n(&r->__owned, 2, __ATOMIC_RELAXED);
    }
  }
  for (hazard_pointer& h : __batch)
    if (__ycxx::__detail::__hp_record* r = __ycxx::__detail::__hp_access::__record(h))
      __atomic_store_n(&r->__owned, 1, __ATOMIC_RELAXED);
  __u.b = {};
}
inline void clear_hazard_pointer_batch(span<hazard_pointer> __batch) noexcept {
  for (hazard_pointer& h : __batch)
    h = hazard_pointer();
}

}} // namespace std
