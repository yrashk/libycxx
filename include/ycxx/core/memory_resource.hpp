// libycxx core: memory_resource ([mem.res.class]), polymorphic_allocator
// ([mem.poly.allocator.class]) and the declarations of the global resources ([mem.res.global]).
//
// Layering (DECISIONS §3): the class definitions are header-only so that <string> (and the
// other containers' pmr:: aliases) can make polymorphic_allocator complete. What needs a
// definition in one place lives in the hosted runtime (src/hosted/memory_resource.cpp): the
// destructor of memory_resource (its key function, so its vtable and type_info are emitted
// there, built with RTTI), new_delete_resource, null_memory_resource and the default resource
// pointer. A freestanding program can name these types but gets a link error if it uses them.
#pragma once

#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/memory_resource_fwd.hpp>
#include <ycxx/core/tuple.hpp>

namespace std::pmr {

// [mem.res.class]
class memory_resource {
  static constexpr size_t max_align = alignof(max_align_t);

public:
  memory_resource() = default;
  memory_resource(const memory_resource&) = default;
  virtual ~memory_resource(); // key function: defined in the hosted runtime
  memory_resource& operator=(const memory_resource&) = default;

  void* allocate(size_t bytes, size_t alignment = max_align) { return do_allocate(bytes, alignment); }
  void deallocate(void* p, size_t bytes, size_t alignment = max_align) { do_deallocate(p, bytes, alignment); }
  bool is_equal(const memory_resource& other) const noexcept { return do_is_equal(other); }

private:
  virtual void* do_allocate(size_t bytes, size_t alignment) = 0;
  virtual void do_deallocate(void* p, size_t bytes, size_t alignment) = 0;
  virtual bool do_is_equal(const memory_resource& other) const noexcept = 0;
};

// [mem.res.eq]
inline bool operator==(const memory_resource& a, const memory_resource& b) noexcept {
  return __builtin_addressof(a) == __builtin_addressof(b) || a.is_equal(b);
}

// [mem.res.global] (hosted runtime)
memory_resource* new_delete_resource() noexcept;
memory_resource* null_memory_resource() noexcept;
memory_resource* set_default_resource(memory_resource* r) noexcept;
memory_resource* get_default_resource() noexcept;

} // namespace std::pmr

namespace ycxx::detail {
// Deallocates a polymorphic_allocator::new_object allocation unless dismissed (the
// constructor did not throw).
template <class Alloc, class T>
struct new_object_guard {
  Alloc& alloc;
  T* p;
  ~new_object_guard() {
    if (p != nullptr)
      alloc.deallocate_object(p);
  }
};
} // namespace ycxx::detail

namespace std::pmr {

// [mem.poly.allocator.class]. The default template argument is in memory_resource_fwd.hpp.
template <class Tp>
class polymorphic_allocator {
  memory_resource* memory_rsrc;

  template <class T>
  static constexpr void check_count(size_t n) {
    if (n > static_cast<size_t>(-1) / sizeof(T))
      ycxx::detail::throw_bad_array_new_length();
  }

public:
  using value_type = Tp;

  polymorphic_allocator() noexcept : memory_rsrc(std::pmr::get_default_resource()) {}
  polymorphic_allocator(memory_resource* r) : memory_rsrc(r) {
    ycxx::detail::precondition(r != nullptr, "polymorphic_allocator: null memory resource");
  }
  polymorphic_allocator(const polymorphic_allocator& other) = default;
  template <class U>
  polymorphic_allocator(const polymorphic_allocator<U>& other) noexcept : memory_rsrc(other.resource()) {}
  polymorphic_allocator& operator=(const polymorphic_allocator&) = delete;

  // [mem.poly.allocator.mem]
  [[nodiscard]] Tp* allocate(size_t n) {
    check_count<Tp>(n);
    return static_cast<Tp*>(memory_rsrc->allocate(n * sizeof(Tp), alignof(Tp)));
  }
  void deallocate(Tp* p, size_t n) { memory_rsrc->deallocate(p, n * sizeof(Tp), alignof(Tp)); }

  [[nodiscard]] void* allocate_bytes(size_t nbytes, size_t alignment = alignof(max_align_t)) {
    return memory_rsrc->allocate(nbytes, alignment);
  }
  void deallocate_bytes(void* p, size_t nbytes, size_t alignment = alignof(max_align_t)) {
    memory_rsrc->deallocate(p, nbytes, alignment);
  }

  template <class T>
  [[nodiscard]] T* allocate_object(size_t n = 1) {
    check_count<T>(n);
    return static_cast<T*>(allocate_bytes(n * sizeof(T), alignof(T)));
  }
  template <class T>
  void deallocate_object(T* p, size_t n = 1) {
    deallocate_bytes(p, n * sizeof(T), alignof(T));
  }

  template <class T, class... CtorArgs>
  [[nodiscard]] T* new_object(CtorArgs&&... ctor_args) {
    T* p = allocate_object<T>();
    ycxx::detail::new_object_guard<polymorphic_allocator, T> guard{*this, p};
    construct(p, static_cast<CtorArgs&&>(ctor_args)...);
    guard.p = nullptr;
    return p;
  }
  template <class T>
  void delete_object(T* p) {
    destroy(p);
    deallocate_object(p);
  }

  // Uses-allocator construction with *this ([allocator.uses.construction]); the pair
  // overloads of uses_allocator_construction_args pass the allocator to both members.
  template <class T, class... Args>
  void construct(T* p, Args&&... args) {
    std::uninitialized_construct_using_allocator(p, *this, static_cast<Args&&>(args)...);
  }
  template <class T>
  void destroy(T* p) {
    p->~T();
  }

  polymorphic_allocator select_on_container_copy_construction() const { return polymorphic_allocator(); }
  memory_resource* resource() const { return memory_rsrc; }

  friend bool operator==(const polymorphic_allocator& a, const polymorphic_allocator& b) noexcept {
    return *a.resource() == *b.resource();
  }
};

// [mem.poly.allocator.eq]
template <class T1, class T2>
bool operator==(const polymorphic_allocator<T1>& a, const polymorphic_allocator<T2>& b) noexcept {
  return *a.resource() == *b.resource();
}

} // namespace std::pmr
