// Storage for an object destroyed from inside one of its own operations: an operation state that
// owns a stop source and is destroyed by its receiver's completion while the source's
// request_stop() still runs, or the stop source itself destroyed by one of its callbacks.
// destroy() ends the object's lifetime and then sets every bit of its bytes, so an operation that
// goes on reading the dead object cannot pass by finding it intact: a stop state's list lock
// reads as taken and the operation spins, which the test's watchdog() turns into a failure (the
// sanitizers see nothing here, the storage itself stays alive; the heap scenarios are theirs).
#pragma once

#include <cstddef>
#include <cstring>
#include <new>
#include <utility>

template <class T>
class destroyed_storage {
  alignas(T) unsigned char bytes_[sizeof(T)];
  T* obj_ = nullptr;

public:
  destroyed_storage() = default;
  destroyed_storage(const destroyed_storage&) = delete;
  destroyed_storage& operator=(const destroyed_storage&) = delete;
  ~destroyed_storage() {
    if (obj_)
      obj_->~T();
  }

  template <class... Args>
  T& emplace(Args&&... args) {
    obj_ = ::new (static_cast<void*>(bytes_)) T(std::forward<Args>(args)...);
    return *obj_;
  }
  // From a factory returning a prvalue: for immovable objects (operation states).
  template <class F>
  T& emplace_with(F f) {
    obj_ = ::new (static_cast<void*>(bytes_)) T(f());
    return *obj_;
  }
  bool alive() const noexcept { return obj_ != nullptr; }
  T& operator*() noexcept { return *obj_; }
  T* operator->() noexcept { return obj_; }

  void destroy() {
    T* p = obj_;
    obj_ = nullptr;
    p->~T();
    std::memset(bytes_, 0xff, sizeof bytes_);
  }
};
