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

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// The link of a retired object (the base of every hazard_pointer_obj_base).
struct hp_retired_node {
  hp_retired_node* hp_next_;
  const void* hp_object_;                          // the T object the hazard pointers name
  void (*hp_reclaim_)(hp_retired_node*) noexcept; // invokes the deleter
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
using adl_free::hp_retired_node;

// A hazard pointer: the value its owner publishes, and whether a hazard_pointer owns it.
struct hp_record {
  const void* value;  // atomic
  hp_record* next;    // immutable once the record is in the list
  ycxx_pal_u32 owned; // atomic
};

// ---- the hosted runtime (src/hosted/hazard_pointer.cpp) ---------------------------------------
// An unowned record, now owned by the caller; throws bad_alloc if a new one cannot be allocated.
hp_record* hp_acquire();
void hp_release(hp_record* r) noexcept;
// Retires n (its object and reclaim function are set); may reclaim retired objects.
void hp_retire(hp_retired_node* n) noexcept;

// [saferecl.hp.general]/2: exactly one public, non-virtual base hazard_pointer_obj_base<T, D>.
template <class T, class Base>
concept hp_protectable_via = std::is_base_of_v<Base, T> && requires(T* p) { static_cast<Base*>(p); } &&
                             requires(Base* b) { static_cast<T*>(b); };

template <class T>
concept hazard_protectable = requires { typename T::ycxx_hp_base; } &&
                             std::is_same_v<typename T::ycxx_hp_base::ycxx_hp_object, std::remove_cv_t<T>> &&
                             hp_protectable_via<std::remove_cv_t<T>, typename T::ycxx_hp_base>;

struct hp_access;

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [saferecl.hp.base]
template <class T, class D = default_delete<T>>
class hazard_pointer_obj_base : ycxx::adl_free::hp_retired_node {
  [[no_unique_address]] D deleter_;

  static void reclaim(ycxx::adl_free::hp_retired_node* n) noexcept {
    hazard_pointer_obj_base* self = static_cast<hazard_pointer_obj_base*>(n);
    self->deleter_(static_cast<T*>(self));
  }

public:
  // For the hazard-protectable check: the base class of T (ambiguous, so absent, when T has
  // several hazard_pointer_obj_base bases).
  using ycxx_hp_base = hazard_pointer_obj_base;
  using ycxx_hp_object = T;

  void retire(D d = D()) noexcept {
    static_assert(ycxx::detail::hazard_protectable<T>, "hazard_pointer_obj_base::retire: T is not hazard-protectable");
    deleter_ = static_cast<D&&>(d);
    T* x = static_cast<T*>(this);
    this->hp_object_ = static_cast<const void*>(x);
    this->hp_reclaim_ = &reclaim;
    ycxx::detail::hp_retire(this);
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
  ycxx::detail::hp_record* rec_ = nullptr;

  explicit hazard_pointer(ycxx::detail::hp_record* r) noexcept : rec_(r) {}
  friend struct ycxx::detail::hp_access;

  void set(const void* p) noexcept {
    ycxx::detail::precondition(rec_ != nullptr, "hazard_pointer: *this is empty");
    __atomic_store_n(&rec_->value, p, __ATOMIC_SEQ_CST);
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
  }

public:
  hazard_pointer() noexcept = default;
  hazard_pointer(hazard_pointer&& other) noexcept : rec_(other.rec_) { other.rec_ = nullptr; }
  hazard_pointer& operator=(hazard_pointer&& other) noexcept {
    if (this != __builtin_addressof(other)) {
      if (rec_)
        ycxx::detail::hp_release(rec_);
      rec_ = other.rec_;
      other.rec_ = nullptr;
    }
    return *this;
  }
  ~hazard_pointer() {
    if (rec_)
      ycxx::detail::hp_release(rec_);
  }

  [[nodiscard]] bool empty() const noexcept { return rec_ == nullptr; }

  template <class T>
  T* protect(const atomic<T*>& src) noexcept {
    T* ptr = src.load(memory_order::relaxed);
    while (!try_protect(ptr, src)) {
    }
    return ptr;
  }
  template <class T>
  bool try_protect(T*& ptr, const atomic<T*>& src) noexcept {
    static_assert(ycxx::detail::hazard_protectable<T>, "hazard_pointer::try_protect: T is not hazard-protectable");
    T* const old = ptr;
    reset_protection(old);
    ptr = src.load(memory_order::acquire);
    if (old != ptr) {
      reset_protection();
      return false;
    }
    return true;
  }
  template <class T>
  void reset_protection(const T* ptr) noexcept {
    static_assert(ycxx::detail::hazard_protectable<T>,
                  "hazard_pointer::reset_protection: T is not hazard-protectable");
    set(static_cast<const void*>(ptr));
  }
  void reset_protection(nullptr_t = nullptr) noexcept {
    ycxx::detail::precondition(rec_ != nullptr, "hazard_pointer: *this is empty");
    __atomic_store_n(&rec_->value, static_cast<const void*>(nullptr), __ATOMIC_RELEASE);
  }
  void swap(hazard_pointer& other) noexcept {
    ycxx::detail::hp_record* r = rec_;
    rec_ = other.rec_;
    other.rec_ = r;
  }
};

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
struct hp_access {
  static std::hazard_pointer make(hp_record* r) noexcept { return std::hazard_pointer(r); }
  static hp_record*& record(std::hazard_pointer& h) noexcept { return h.rec_; }
};
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [saferecl.hp.holder.nonmem]
[[nodiscard]] inline hazard_pointer make_hazard_pointer() {
  return ycxx::detail::hp_access::make(ycxx::detail::hp_acquire());
}
inline void swap(hazard_pointer& a, hazard_pointer& b) noexcept { a.swap(b); }

inline void make_hazard_pointer_batch(span<hazard_pointer> batch) {
  // All or nothing: a record this call gave an element is marked (owned == 2) until every
  // element has one; if an acquisition throws, the marked ones are given back.
  struct undo {
    span<hazard_pointer> b;
    ~undo() {
      for (hazard_pointer& h : b) {
        ycxx::detail::hp_record*& r = ycxx::detail::hp_access::record(h);
        if (r && __atomic_load_n(&r->owned, __ATOMIC_RELAXED) == 2) {
          ycxx::detail::hp_release(r);
          r = nullptr;
        }
      }
    }
  } u{batch};
  for (hazard_pointer& h : batch) {
    ycxx::detail::hp_record*& r = ycxx::detail::hp_access::record(h);
    if (!r) {
      r = ycxx::detail::hp_acquire();
      __atomic_store_n(&r->owned, 2, __ATOMIC_RELAXED);
    }
  }
  for (hazard_pointer& h : batch)
    if (ycxx::detail::hp_record* r = ycxx::detail::hp_access::record(h))
      __atomic_store_n(&r->owned, 1, __ATOMIC_RELAXED);
  u.b = {};
}
inline void clear_hazard_pointer_batch(span<hazard_pointer> batch) noexcept {
  for (hazard_pointer& h : batch)
    h = hazard_pointer();
}

} // namespace std
