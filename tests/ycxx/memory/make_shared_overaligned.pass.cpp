// [util.smartptr.shared.create]/3: "Allocates memory for an object of type T (or U[N] ...)";
// /7.4, /7.6: the object is created by "::new(pv) U(...)" where "pv ... points to storage
// suitable to hold an object of type U" -- so over-aligned types (alignof > the default
// new alignment) are correctly aligned, for single objects, every array element (U[] and
// U[N]) and the _for_overwrite forms. /7.5: allocate_shared uses "a2 of type A2 ... a
// potentially rebound copy of the allocator a"; storage comes from that allocator (the
// stateful allocator below counts and checks alignment itself), and is returned to it when
// the last owner and weak reference are gone. An enable_shared_from_this base works too.
#include <memory>
#include <cstddef>
#include <cstdint>
#include <new>
#include <vector>
#include "check.hpp"

struct alignas(256) Over {
  unsigned char c[3];
};
struct alignas(128) OverE : std::enable_shared_from_this<OverE> {
  int x = 9;
};
static_assert(alignof(Over) > __STDCPP_DEFAULT_NEW_ALIGNMENT__);

template <class T>
bool aligned(const T* p, std::size_t a) {
  return reinterpret_cast<std::uintptr_t>(p) % a == 0;
}

static int allocs = 0, deallocs = 0;
template <class T>
struct Counting {
  using value_type = T;
  int id;
  explicit Counting(int i) : id(i) {}
  template <class U>
  Counting(const Counting<U>& o) : id(o.id) {}
  T* allocate(std::size_t n) {
    ++allocs;
    return static_cast<T*>(::operator new(n * sizeof(T), std::align_val_t(alignof(T))));
  }
  void deallocate(T* p, std::size_t n) {
    CHECK(id == 7);  // a copy of the original allocator
    ++deallocs;
    ::operator delete(p, n * sizeof(T), std::align_val_t(alignof(T)));
  }
  template <class U>
  bool operator==(const Counting<U>& o) const { return id == o.id; }
};

int main() {
  std::vector<std::shared_ptr<Over>> keep;
  for (int i = 0; i < 16; ++i) {
    keep.push_back(std::make_shared<Over>());
    CHECK(aligned(keep.back().get(), 256));
  }
  auto arr = std::make_shared<Over[]>(5);
  for (int i = 0; i < 5; ++i) CHECK(aligned(&arr[i], 256));
  auto fixed = std::make_shared<Over[3]>();
  for (int i = 0; i < 3; ++i) CHECK(aligned(&fixed[i], 256));
  auto ow = std::make_shared_for_overwrite<Over>();
  CHECK(aligned(ow.get(), 256));
  auto owa = std::make_shared_for_overwrite<Over[]>(4);
  CHECK(aligned(&owa[3], 256));
  auto e = std::make_shared<OverE>();
  CHECK(aligned(e.get(), 128) && e->shared_from_this() == e);
  {
    std::weak_ptr<Over> w;
    {
      auto a = std::allocate_shared<Over>(Counting<Over>(7));
      CHECK(aligned(a.get(), 256));
      auto b = std::allocate_shared<Over[]>(Counting<Over>(7), 7);
      for (int i = 0; i < 7; ++i) CHECK(aligned(&b[i], 256));
      auto c = std::allocate_shared_for_overwrite<Over[2]>(Counting<int>(7));
      CHECK(aligned(&c[1], 256));
      auto d = std::allocate_shared<OverE>(Counting<char>(7));
      CHECK(aligned(d.get(), 128) && d->shared_from_this() == d);
      w = a;
      CHECK(allocs == 4 && deallocs == 0);
    }
    CHECK(deallocs == 3);  // the control block with a weak reference is still there
  }
  CHECK(allocs == 4 && deallocs == 4);
  return 0;
}
