// FLAGS: -freflection
// XFAIL-COMPILER: clang  Clang 23 has no reflection (P2996)
// The members of <meta>'s aggregates that a program spells (DECISIONS §2: tools/uglify.py once
// renamed `annotations`, `bytes` and `bits`, which the index of library names misses):
// [meta.reflection.define.aggregate] (synopsis): data_member_options has the members name,
// alignment, bit_width, no_unique_address and annotations, which a program names in designated
// initializers; [meta.reflection.layout] (synopsis), /1, /3: member_offset has the members bytes
// and bits, total_bits() is bytes * CHAR_BIT + bits, and offset_of returns {V / CHAR_BIT,
// V % CHAR_BIT}.
#include <meta>
#include <cstddef>
#include <vector>

namespace m = std::meta;

// Every member of data_member_options, by designator and by name.
consteval bool options_have_their_members() {
  m::data_member_options o{.name = "x", .alignment = 8, .bit_width = 3, .no_unique_address = true,
                           .annotations = {m::reflect_constant(1), m::reflect_constant(2)}};
  return o.name.has_value() && *o.alignment == 8 && *o.bit_width == 3 && o.no_unique_address &&
         o.annotations.size() == 2 && o.annotations[1] == m::reflect_constant(2);
}
static_assert(options_have_their_members());
static_assert(m::data_member_options{}.annotations.empty());

struct S {
  char c;
  unsigned lo : 3;
  unsigned hi : 5;
};
// member_offset's bytes and bits, read and initialized by name.
static_assert(m::offset_of(^^S::c).bytes == 0 && m::offset_of(^^S::c).bits == 0);
static_assert(m::offset_of(^^S::hi).bytes * 8 + m::offset_of(^^S::hi).bits ==
              m::offset_of(^^S::lo).bytes * 8 + m::offset_of(^^S::lo).bits + 3);
constexpr m::member_offset off{.bytes = 2, .bits = 5};
static_assert(off.total_bits() == 21);

int main() {}
