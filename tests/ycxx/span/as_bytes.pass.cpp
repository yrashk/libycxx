// [span.objectrep]: as_bytes returns span<const byte, Extent == dynamic_extent ?
// dynamic_extent : sizeof(ElementType) * Extent>, as_writable_bytes the same with byte;
// both noexcept. as_bytes "Constraints: is_volatile_v<ElementType> is false.";
// as_writable_bytes "Constraints: is_const_v<ElementType> is false and
// is_volatile_v<ElementType> is false."
#include <span>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

template <class S>
concept can_as_bytes = requires(S s) { std::as_bytes(s); };
template <class S>
concept can_as_writable_bytes = requires(S s) { std::as_writable_bytes(s); };

static_assert(can_as_bytes<std::span<int>>);
static_assert(can_as_bytes<std::span<const int, 2>>);
static_assert(!can_as_bytes<std::span<volatile int>>);
static_assert(!can_as_bytes<std::span<const volatile int, 2>>);
static_assert(can_as_writable_bytes<std::span<int>>);
static_assert(!can_as_writable_bytes<std::span<const int>>);
static_assert(!can_as_writable_bytes<std::span<volatile int, 3>>);

static_assert(std::is_same_v<decltype(std::as_bytes(std::span<int>())), std::span<const std::byte>>);
static_assert(
    std::is_same_v<decltype(std::as_bytes(std::declval<std::span<int, 3>>())), std::span<const std::byte, 3 * sizeof(int)>>);
static_assert(std::is_same_v<decltype(std::as_writable_bytes(std::declval<std::span<long, 2>>())),
                             std::span<std::byte, 2 * sizeof(long)>>);
static_assert(std::is_same_v<decltype(std::as_writable_bytes(std::span<short>())), std::span<std::byte>>);
static_assert(std::is_same_v<decltype(std::as_bytes(std::declval<std::span<int, 0>>())), std::span<const std::byte, 0>>);
static_assert(noexcept(std::as_bytes(std::span<int>())));
static_assert(noexcept(std::as_writable_bytes(std::span<int>())));

int main() {
  unsigned int a[2] = {0x01020304u, 0u};
  std::span<unsigned int, 2> s(a);
  auto b = std::as_bytes(s);
  CHECK(static_cast<const void*>(b.data()) == static_cast<const void*>(a));
  CHECK(b.size() == sizeof a);
  auto w = std::as_writable_bytes(std::span<unsigned int>(a));
  CHECK(static_cast<void*>(w.data()) == static_cast<void*>(a));
  CHECK(w.size() == sizeof a);
  for (std::byte& x : w.last(sizeof(unsigned int))) x = std::byte{0xFF};
  CHECK(a[1] == ~0u);
  CHECK(a[0] == 0x01020304u);
  return 0;
}
