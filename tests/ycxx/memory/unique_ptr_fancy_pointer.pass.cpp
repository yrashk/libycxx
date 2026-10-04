// [unique.ptr.single.general]/3: "If the qualified-id remove_reference_t<D>::pointer is valid
// and denotes a type, then unique_ptr<T, D>::pointer shall be a synonym for
// remove_reference_t<D>::pointer." That type "shall meet the Cpp17NullablePointer
// requirements". [unique.ptr.single.observers]: get() returns the stored pointer, operator
// bool is get() != nullptr; [unique.ptr.single.modifiers]: release() returns the stored
// pointer and stores pointer(); reset(p) stores p and then calls the deleter on the old value
// if it compares unequal to nullptr; [unique.ptr.single.dtor]: the destructor calls
// get_deleter()(get()) if get() != nullptr.
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"

// A Cpp17NullablePointer handle: an index into a table, -1 means null.
struct Handle {
  int idx = -1;
  Handle() = default;
  Handle(std::nullptr_t) {}
  explicit Handle(int i) : idx(i) {}
  explicit operator bool() const { return idx != -1; }  // as if p != nullptr
  friend bool operator==(Handle a, Handle b) { return a.idx == b.idx; }
  friend bool operator==(Handle a, std::nullptr_t) { return a.idx == -1; }
};

static int closed[8];
static int close_calls = 0;

struct Closer {
  using pointer = Handle;
  void operator()(Handle h) const {
    ++close_calls;
    ++closed[h.idx];
  }
};

using UP = std::unique_ptr<int, Closer>;
static_assert(std::is_same_v<UP::pointer, Handle>);
static_assert(std::is_same_v<UP::element_type, int>);

int main() {
  {
    UP u;
    CHECK(!u);
    CHECK(u.get() == nullptr);
  }
  CHECK(close_calls == 0);  // null pointer: deleter not called

  {
    UP u(Handle(1));
    CHECK(static_cast<bool>(u));
    CHECK(u.get().idx == 1);
    u.reset(Handle(2));
    CHECK(closed[1] == 1 && u.get().idx == 2);
    Handle h = u.release();
    CHECK(h.idx == 2 && !u && u.get() == nullptr);
    u.reset(Handle(3));
    UP v(std::move(u));
    CHECK(!u && v.get().idx == 3);
    u = std::move(v);
    CHECK(u.get().idx == 3);
    u.reset();
    CHECK(closed[3] == 1 && !u);
    u.reset(Handle(4));
    u = nullptr;
    CHECK(closed[4] == 1);
    u.reset(Handle(5));
  }
  CHECK(closed[5] == 1);
  CHECK(closed[2] == 0);  // released, never closed
  CHECK(close_calls == 3 + 0 + 1);
  return 0;
}
