// [ptrtag.bits]/1: "inline constexpr unsigned max_pointer_bits_available = see below;" "The
// implementation-defined limit of usable bits for pointer tagging."
// [ptrtag.bits]/3-7: constexpr unsigned pointer_bits_available(size_t alignment);
// "Constant When: Precondition is met." "Preconditions: alignment is a power of two."
// "Returns: The implementation-defined number of unused bits in a pointer pointing to a
// hypothetical object with alignment alignment." "Recommended practice: The result is non-zero
// for alignment values larger than 1." Note 1: "On most platforms, the value is the minimum of
// countr_zero(alignment) and max_pointer_bits_available."
// libycxx (DECISIONS §9): max_pointer_bits_available is the pointer width minus 1.
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::max_pointer_bits_available), const unsigned>);
static_assert(std::max_pointer_bits_available == sizeof(void*) * 8 - 1);
static_assert(std::is_same_v<decltype(std::pointer_bits_available(std::size_t(8))), unsigned>);

// Usable in constant expressions when the precondition holds.
static_assert(std::pointer_bits_available(1) == 0);
static_assert(std::pointer_bits_available(2) == 1);
static_assert(std::pointer_bits_available(4) == 2);
static_assert(std::pointer_bits_available(8) == 3);
static_assert(std::pointer_bits_available(4096) == 12);
static_assert(std::pointer_bits_available(alignof(std::max_align_t)) ==
              static_cast<unsigned>(std::countr_zero(alignof(std::max_align_t))));
// The largest power of two of size_t: countr_zero is the width - 1, the limit.
constexpr std::size_t top = std::size_t(1) << (sizeof(std::size_t) * 8 - 1);
static_assert(std::pointer_bits_available(top) == std::max_pointer_bits_available);
// /6 recommended practice: non-zero above 1.
static_assert(std::pointer_bits_available(16) > 0);

// /3: not a constant expression when the precondition is violated.
template <std::size_t A>
concept constant_bits = requires { typename std::integral_constant<unsigned, std::pointer_bits_available(A)>; };
static_assert(constant_bits<64>);
static_assert(!constant_bits<3>);
static_assert(!constant_bits<0>);
static_assert(!constant_bits<24>);

// [memory.syn]: freestanding; usable as a default argument of pointer_tag_pair's BitsRequested.
static_assert(std::pointer_tag_pair<std::uint64_t*>::bits_requested == std::pointer_bits_available(alignof(std::uint64_t)));
static_assert(std::pointer_tag_pair<char*>::bits_requested == 0);

int main() {
  volatile std::size_t a = 32;
  CHECK(std::pointer_bits_available(a) == 5);
  a = 1;
  CHECK(std::pointer_bits_available(a) == 0);
  return 0;
}
