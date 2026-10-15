// libycxx's documented SBO policy, including both global allocation paths and fallback.
// REQUIRES: exceptions
#include <ycxx/functional.hpp>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include "check.hpp"

static int allocations = 0;
void* operator new(std::size_t n) {
  ++allocations;
  if (void* p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}
void* operator new(std::size_t n, std::align_val_t a) {
  ++allocations;
  auto alignment = static_cast<std::size_t>(a);
  auto bytes = (n + alignment - 1) / alignment * alignment;
  if (void* p = std::aligned_alloc(alignment, bytes ? bytes : alignment)) return p;
  throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }

template <std::size_t Words>
struct Payload {
  void* data[Words]{};
  int operator()() const { return static_cast<int>(Words); }
};
struct alignas(std::max_align_t) Aligned {
  int n = 9;
  int operator()() const { return n; }
};
struct alignas(2 * alignof(std::max_align_t)) Overaligned {
  int operator()() const { return 8; }
};
struct ThrowingMove {
  ThrowingMove() = default;
  ThrowingMove(const ThrowingMove&) = default;
  ThrowingMove(ThrowingMove&&) noexcept(false) {}
  int operator()() const { return 7; }
};

template <class W, class T>
void inline_lifecycle(const T& target, int result) {
  int before = allocations;
  {
    W a(target);
    W b = [&] {
      if constexpr (std::is_copy_constructible_v<W>) return W(a);
      else return W(target);
    }();
    W c(std::move(a));
    CHECK(!a && b() == result && c() == result);
    a = std::move(b);
    CHECK(!b && a() == result);
    a.swap(c);
    c = nullptr;
  }
  CHECK(allocations == before);
}

int main() {
  using W = std::function<int()>;
  static_assert(ycxx::uses_inline_storage_v<W, Payload<4>>);
  static_assert(!ycxx::uses_inline_storage_v<W, Payload<5>>);
  static_assert(ycxx::uses_inline_storage_v<W, Aligned>);
  static_assert(!ycxx::uses_inline_storage_v<W, Overaligned>);
  static_assert(!ycxx::uses_inline_storage_v<W, ThrowingMove>);
  inline_lifecycle<W>(Payload<4>{}, 4);
  inline_lifecycle<W>(Aligned{}, 9);
  inline_lifecycle<std::move_only_function<int()>>(Payload<4>{}, 4);
  inline_lifecycle<std::copyable_function<int()>>(Aligned{}, 9);

  auto p = std::make_shared<int>(1), q = std::make_shared<int>(2);
  auto pair = [p, q] { return *p + *q; };
  static_assert(ycxx::uses_inline_storage_v<W, decltype(pair)>);
  inline_lifecycle<W>(pair, 3);
  CHECK(p.use_count() == 2 && q.use_count() == 2);
  auto text = [s = std::string("hello")] { return static_cast<int>(s.size()); };
  static_assert(ycxx::uses_inline_storage_v<W, decltype(text)>);
  inline_lifecycle<W>(text, 5);

  auto fallback = []<class T>(T target) {
    int before = allocations;
    {
      W f(std::move(target));
      CHECK(allocations == before + 1);
      W g(std::move(f));
      CHECK(!f && g());
      CHECK(allocations == before + 1);
    }
  };
  fallback(Payload<5>{});
  fallback(Overaligned{});
  fallback(ThrowingMove{});

  // A larger user-selected inline buffer still uses neither global allocation path.
  inline_lifecycle<ycxx::inplace_function<int(), sizeof(Payload<8>)>>(Payload<8>{}, 8);
  inline_lifecycle<ycxx::inplace_function<int(), sizeof(Overaligned), alignof(Overaligned)>>(Overaligned{}, 8);
}
