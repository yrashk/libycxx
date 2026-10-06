// [mdspan.accessor.reqmts]: an accessor's data_handle_type "need not be element_type*" (Note 1)
// and its reference "need not be element_type&" (Note 2; it models
// common_reference_with<reference&&, element_type&>); offset(p, i) returns a handle q of
// offset_policy with b.access(q, j) the element a.access(p, i + j).
// [mdspan.mdspan.overview]/2-/3: mdspan stores the accessor and data handle; [mdspan.mdspan.members]:
// operator[] returns acc_.access(ptr_, map_(I...)) (a proxy here), data_handle() and accessor()
// return the stored ones, size() and empty() come from the extents.
// [mdspan.mdspan.cons]: mdspan(p, exts...) value-initializes the accessor, and mdspan(p, m, a)
// takes it; [mdspan.sub.sub]/7: submdspan offsets the handle through the accessor.
// The accessor here packs bools as bits of an array of bytes.
#include <mdspan>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct bit_handle {
  std::uint8_t* bytes = nullptr;
  std::size_t bit = 0;  // offset in bits
};

class bit_ref {
  std::uint8_t* byte_;
  std::uint8_t mask_;

 public:
  constexpr bit_ref(std::uint8_t* b, std::uint8_t m) : byte_(b), mask_(m) {}
  constexpr operator bool() const { return (*byte_ & mask_) != 0; }
  constexpr const bit_ref& operator=(bool v) const {
    if (v)
      *byte_ |= mask_;
    else
      *byte_ &= static_cast<std::uint8_t>(~mask_);
    return *this;
  }
  constexpr const bit_ref& operator=(const bit_ref& o) const { return *this = bool(o); }
};
template <template <class> class TQ, template <class> class UQ>
struct std::basic_common_reference<bit_ref, bool, TQ, UQ> {
  using type = bool;
};
template <template <class> class TQ, template <class> class UQ>
struct std::basic_common_reference<bool, bit_ref, TQ, UQ> {
  using type = bool;
};

struct bit_accessor {
  using element_type = bool;
  using reference = bit_ref;
  using data_handle_type = bit_handle;
  using offset_policy = bit_accessor;
  int id = 0;  // to see which accessor object an mdspan holds
  constexpr reference access(data_handle_type p, std::size_t i) const {
    std::size_t b = p.bit + i;
    return bit_ref(p.bytes + b / 8, static_cast<std::uint8_t>(1u << (b % 8)));
  }
  constexpr data_handle_type offset(data_handle_type p, std::size_t i) const { return {p.bytes, p.bit + i}; }
};
static_assert(std::copyable<bit_accessor> && std::is_nothrow_move_constructible_v<bit_accessor>);
static_assert(std::common_reference_with<bit_ref&&, bool&>);

using E = std::extents<int, 3, 5>;
using M = std::mdspan<bool, E, std::layout_right, bit_accessor>;
static_assert(std::is_same_v<M::reference, bit_ref> && std::is_same_v<M::data_handle_type, bit_handle>);
static_assert(std::is_same_v<M::element_type, bool> && std::is_same_v<M::value_type, bool>);

constexpr bool run() {
  std::uint8_t bytes[2] = {};
  M m(bit_handle{bytes, 0});
  if (m.accessor().id != 0 || m.size() != 15 || m.empty()) return false;
  static_assert(std::is_same_v<decltype(m[0, 0]), bit_ref>);
  m[0, 0] = true;
  m[1, 2] = true;  // index 7
  m[2, 4] = true;  // index 14
  if (bytes[0] != 0x81 || bytes[1] != 0x40) return false;
  if (!m[1, 2] || m[1, 3]) return false;
  m[1, 2] = m[0, 1];  // proxy-to-proxy assignment writes through
  if (bytes[0] != 0x01) return false;

  // a stored accessor object and handle
  M m2(bit_handle{bytes, 3}, E{}, bit_accessor{7});
  if (m2.accessor().id != 7 || m2.data_handle().bit != 3 || m2.data_handle().bytes != bytes) return false;
  m2[0, 0] = true;  // bit 3
  if (bytes[0] != 0x09) return false;

  // submdspan: the handle is advanced through offset, the accessor converted to offset_policy
  auto row = std::submdspan(m, 2, std::full_extent);
  static_assert(std::is_same_v<decltype(row)::accessor_type, bit_accessor>);
  if (row.data_handle().bit != 10 || row.extent(0) != 5 || !row[4] || row[3]) return false;
  row[0] = true;  // bit 10
  if (bytes[1] != 0x44) return false;
  auto col = std::submdspan(m, std::full_extent, 4);
  if (!col[2] || col[0] || col.stride(0) != 5) return false;
  return true;
}

int main() {
  CHECK(run());
  static_assert(run());
  return 0;
}
