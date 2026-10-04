// [new.delete.single]/1-3: operator new(size_t size, align_val_t alignment): "Effects: The
// allocation functions called by a new-expression to allocate size bytes of storage. The second
// form is called for a type with new-extended alignment, and the first form is called
// otherwise." "Returns: A non-null pointer to suitably aligned storage, or else throw a bad_alloc
// exception." [basic.stc.dynamic.allocation]/2: the storage has at least the requested size and
// is aligned to the requested alignment; "Even if the size of the space requested is zero, the
// request can fail. If the request succeeds, the value returned ... shall be a non-null pointer
// value p0 different from any previously returned value p1, unless that value p1 was
// subsequently passed to a replaceable deallocation function"; [new.delete.single]/5: the
// nothrow forms return a null pointer where the throwing forms would throw.
// [basic.align]/4: every alignment value is a non-negative integral power of two; any power of
// two is a valid argument (alignments smaller than __STDCPP_DEFAULT_NEW_ALIGNMENT__ included).
// Sizes that are not a multiple of the alignment, and sizes so large that rounding them up to
// the alignment would wrap around, must not yield undersized storage.
#include <new>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include "check.hpp"

static bool aligned(void* p, std::size_t a) { return reinterpret_cast<std::uintptr_t>(p) % a == 0; }

int main() {
  const std::size_t aligns[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 4096, std::size_t(1) << 16, std::size_t(1) << 20};
  const std::size_t sizes[] = {0, 1, 3, 17, 100, 4097};
  for (std::size_t a : aligns) {
    for (std::size_t n : sizes) {
      void* p = ::operator new(n, std::align_val_t(a));
      void* q = ::operator new(n, std::align_val_t(a));
      CHECK(p != nullptr && q != nullptr && p != q);
      CHECK(aligned(p, a) && aligned(q, a));
      std::memset(p, 0xA5, n);   // the whole requested size is usable
      std::memset(q, 0x5A, n);
      for (std::size_t i = 0; i < n; ++i) CHECK(static_cast<unsigned char*>(p)[i] == 0xA5);
      ::operator delete(q, n, std::align_val_t(a));
      ::operator delete(p, std::align_val_t(a));
      void* r = ::operator new[](n, std::align_val_t(a), std::nothrow);
      CHECK(r != nullptr && aligned(r, a));
      std::memset(r, 1, n);
      ::operator delete[](r, std::align_val_t(a), std::nothrow);
    }
  }
  // Zero-size requests return distinct pointers ([basic.stc.dynamic.allocation]/2).
  void* z1 = ::operator new(0);
  void* z2 = ::operator new(0);
  CHECK(z1 && z2 && z1 != z2);
  ::operator delete(z1);
  ::operator delete(z2);
  void* z3 = ::operator new[](0, std::align_val_t(64));
  void* z4 = ::operator new[](0, std::align_val_t(64));
  CHECK(z3 && z4 && z3 != z4 && aligned(z3, 64) && aligned(z4, 64));
  ::operator delete[](z3, std::align_val_t(64));
  ::operator delete[](z4, std::align_val_t(64));

  // Requests that cannot be satisfied, including ones whose round-up to the alignment wraps.
  const std::size_t max = static_cast<std::size_t>(-1);
  const std::size_t huge[] = {max, max - 1, max - 63, max - 4095, max / 2 + 1};
  for (std::size_t n : huge) {
    for (std::size_t a : {std::size_t(16), std::size_t(64), std::size_t(4096)}) {
      CHECK(::operator new(n, std::align_val_t(a), std::nothrow) == nullptr);
      CHECK(::operator new[](n, std::align_val_t(a), std::nothrow) == nullptr);
      bool threw = false;
      try {
        void* p = ::operator new(n, std::align_val_t(a));
        ::operator delete(p, std::align_val_t(a));
      } catch (const std::bad_alloc&) {
        threw = true;
      }
      CHECK(threw);
      threw = false;
      try {
        void* p = ::operator new[](n, std::align_val_t(a));
        ::operator delete[](p, std::align_val_t(a));
      } catch (const std::bad_alloc&) {
        threw = true;
      }
      CHECK(threw);
    }
    CHECK(::operator new(n, std::nothrow) == nullptr);
    CHECK(::operator new[](n, std::nothrow) == nullptr);
    bool threw = false;
    try {
      void* p = ::operator new(n);
      ::operator delete(p);
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    CHECK(threw);
  }
  return 0;
}
