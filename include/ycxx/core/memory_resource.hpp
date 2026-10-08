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

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace pmr {

// [mem.res.class]
class memory_resource {
  static constexpr size_t __max_align = alignof(max_align_t);

public:
  memory_resource() = default;
  memory_resource(const memory_resource&) = default;
  virtual ~memory_resource(); // key function: defined in the hosted runtime
  memory_resource& operator=(const memory_resource&) = default;

  void* allocate(size_t bytes, size_t alignment = __max_align) { return do_allocate(bytes, alignment); }
  void deallocate(void* p, size_t bytes, size_t alignment = __max_align) { do_deallocate(p, bytes, alignment); }
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

}}} // namespace std::pmr

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// Deallocates a polymorphic_allocator::new_object allocation unless dismissed (the
// constructor did not throw).
template <class _Alloc, class _Tp>
struct __new_object_guard {
  _Alloc& __alloc;
  _Tp* p;
  ~__new_object_guard() {
    if (p != nullptr)
      __alloc.deallocate_object(p);
  }
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace pmr {

// [mem.poly.allocator.class]. The default template argument is in memory_resource_fwd.hpp.
template <class _Tp_>
class polymorphic_allocator {
  memory_resource* __memory_rsrc;

  template <class _Tp>
  static constexpr void __check_count(size_t n) {
    if (n > static_cast<size_t>(-1) / sizeof(_Tp))
      __ycxx::__detail::__throw_bad_array_new_length();
  }

public:
  using value_type = _Tp_;

  polymorphic_allocator() noexcept : __memory_rsrc(std::pmr::get_default_resource()) {}
  polymorphic_allocator(memory_resource* r) : __memory_rsrc(r) {
    __ycxx::__detail::__precondition(r != nullptr, "polymorphic_allocator: null memory resource");
  }
  polymorphic_allocator(const polymorphic_allocator& other) = default;
  template <class _Up>
  polymorphic_allocator(const polymorphic_allocator<_Up>& other) noexcept : __memory_rsrc(other.resource()) {}
  polymorphic_allocator& operator=(const polymorphic_allocator&) = delete;

  // [mem.poly.allocator.mem]
  [[nodiscard]] _Tp_* allocate(size_t n) {
    __check_count<_Tp_>(n);
    return static_cast<_Tp_*>(__memory_rsrc->allocate(n * sizeof(_Tp_), alignof(_Tp_)));
  }
  void deallocate(_Tp_* p, size_t n) { __memory_rsrc->deallocate(p, n * sizeof(_Tp_), alignof(_Tp_)); }

  [[nodiscard]] void* allocate_bytes(size_t __nbytes, size_t alignment = alignof(max_align_t)) {
    return __memory_rsrc->allocate(__nbytes, alignment);
  }
  void deallocate_bytes(void* p, size_t __nbytes, size_t alignment = alignof(max_align_t)) {
    __memory_rsrc->deallocate(p, __nbytes, alignment);
  }

  template <class _Tp>
  [[nodiscard]] _Tp* allocate_object(size_t n = 1) {
    __check_count<_Tp>(n);
    return static_cast<_Tp*>(allocate_bytes(n * sizeof(_Tp), alignof(_Tp)));
  }
  template <class _Tp>
  void deallocate_object(_Tp* p, size_t n = 1) {
    deallocate_bytes(p, n * sizeof(_Tp), alignof(_Tp));
  }

  template <class _Tp, class... _CtorArgs>
  [[nodiscard]] _Tp* new_object(_CtorArgs&&... __ctor_args) {
    _Tp* p = allocate_object<_Tp>();
    __ycxx::__detail::__new_object_guard<polymorphic_allocator, _Tp> __guard{*this, p};
    construct(p, static_cast<_CtorArgs&&>(__ctor_args)...);
    __guard.p = nullptr;
    return p;
  }
  template <class _Tp>
  void delete_object(_Tp* p) {
    destroy(p);
    deallocate_object(p);
  }

  // Uses-allocator construction with *this ([allocator.uses.construction]); the pair
  // overloads of uses_allocator_construction_args pass the allocator to both members.
  template <class _Tp, class... _Args>
  void construct(_Tp* p, _Args&&... __args) {
    std::uninitialized_construct_using_allocator(p, *this, static_cast<_Args&&>(__args)...);
  }
  template <class _Tp>
  void destroy(_Tp* p) {
    p->~_Tp();
  }

  polymorphic_allocator select_on_container_copy_construction() const { return polymorphic_allocator(); }
  memory_resource* resource() const { return __memory_rsrc; }

  friend bool operator==(const polymorphic_allocator& a, const polymorphic_allocator& b) noexcept {
    return *a.resource() == *b.resource();
  }
};

// [mem.poly.allocator.eq]
template <class _T1, class _T2>
bool operator==(const polymorphic_allocator<_T1>& a, const polymorphic_allocator<_T2>& b) noexcept {
  return *a.resource() == *b.resource();
}

}}} // namespace std::pmr
