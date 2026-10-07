// [ptrtag.pair]: class template pointer_tag_pair, at run time.
// [ptrtag.pair.general]/2: "An object of class pointer_tag_pair<Ptr, BitsRequested, TagT>
// represents a pair of pointer value of type Ptr and tag value of type TagT."
// /3: "Each specialization PT of pointer_tag_pair is a trivially copyable type that models
// copyable such that sizeof(PT) is equal to sizeof(Ptr) and alignof(PT) is equal to alignof(Ptr)."
// [ptrtag.pair.cons]/1: default constructor: "pointer() is equal to nullptr and tag() is equal
// to TagT()"; /4: "pointer() is equal to p and tag() is equal to t."
// [ptrtag.pair.overalign]/3: from_overaligned returns ptp with "ptp.pointer() is equal to p and
// ptp.tag() is equal to t."
// [ptrtag.pair.tagops]/1: tagged_pointer_type is cv void* for a pointer to cv U; /2-3:
// DP::from_tagged(tp.tagged_pointer()) gives back the pointer and the tag for any DP whose
// bits_requested holds the tag and fits the pointer's alignment.
// [ptrtag.pair.accessors], [ptrtag.pair.swap]/1: "Exchanges the values of *this and o."
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct alignas(16) Node {
  int v;
};
struct alignas(8) Base {
  long b;
};
struct Derived : Base {}; // standard-layout: Base is pointer-interconvertible with it
union alignas(4) U {
  int i;
  float f;
};
enum class Color : unsigned char { red, green, blue };
enum Plain : unsigned { p0, p1, p2, p3 };

using PT = std::pointer_tag_pair<Node*>;
// Member types ([ptrtag.pair.general]).
static_assert(std::is_same_v<PT::pointer_type, Node*>);
static_assert(std::is_same_v<PT::element_type, Node>);
static_assert(std::is_same_v<PT::tagged_pointer_type, void*>);
static_assert(std::is_same_v<PT::tag_type, unsigned>);
static_assert(PT::bits_requested == 4); // bits-available<Node>: alignment 16
static_assert(std::is_same_v<decltype(PT::bits_requested), const unsigned>);
static_assert(std::is_same_v<std::pointer_tag_pair<const int*>::tagged_pointer_type, const void*>);
static_assert(std::is_same_v<std::pointer_tag_pair<volatile int*>::tagged_pointer_type, volatile void*>);
static_assert(std::is_same_v<std::pointer_tag_pair<const volatile int*>::tagged_pointer_type, const volatile void*>);
static_assert(std::is_same_v<std::pointer_tag_pair<const volatile int*>::element_type, const volatile int>);
static_assert(std::is_same_v<std::pointer_tag_pair<void*, 3>::element_type, void>);
static_assert(std::is_same_v<std::pointer_tag_pair<int*>, std::pointer_tag_pair<int*, 2, unsigned>>);

// /3: layout and copyability.
template <class P>
constexpr bool layout_ok = std::is_trivially_copyable_v<P> && std::copyable<P> &&
                           sizeof(P) == sizeof(typename P::pointer_type) &&
                           alignof(P) == alignof(typename P::pointer_type);
static_assert(layout_ok<PT>);
static_assert(layout_ok<std::pointer_tag_pair<const char*, 0>>);
static_assert(layout_ok<std::pointer_tag_pair<void*, 3, Color>>);
static_assert(layout_ok<std::pointer_tag_pair<Base*, 3, bool>>);
static_assert(layout_ok<std::pointer_tag_pair<std::uint64_t*, 3, std::uint64_t>>);

// noexcept as declared; the constructors are not declared noexcept ("Throws: Nothing.").
static_assert(std::is_nothrow_default_constructible_v<PT>);
static_assert(noexcept(std::declval<PT&>().pointer()) && noexcept(std::declval<PT&>().tag()));
static_assert(noexcept(std::declval<PT&>().tagged_pointer()) && noexcept(PT::from_tagged(nullptr)));
static_assert(noexcept(std::declval<PT&>().swap(std::declval<PT&>())));
static_assert(std::is_same_v<decltype(std::declval<const PT&>().tagged_pointer()), void*>);
static_assert(std::is_same_v<decltype(PT::from_tagged(nullptr)), PT>);
static_assert(std::is_same_v<decltype(std::declval<const PT&>().pointer()), Node*>);
static_assert(std::is_same_v<decltype(std::declval<const PT&>().tag()), unsigned>);
static_assert(std::is_same_v<decltype(PT::from_overaligned<16>(static_cast<Node*>(nullptr), 0u)), PT>);

int main() {
  // [ptrtag.pair.cons]/1
  {
    PT d;
    CHECK(d.pointer() == nullptr);
    CHECK(d.tag() == 0u);
    std::pointer_tag_pair<int*, 2, Color> dc;
    CHECK(dc.pointer() == nullptr && dc.tag() == Color());
  }
  // /4: every tag value that fits, at each element of an over-aligned array.
  {
    Node nodes[4];
    for (Node& n : nodes)
      for (unsigned t = 0; t < 16; ++t) {
        PT p(&n, t);
        CHECK(p.pointer() == &n);
        CHECK(p.tag() == t);
      }
    PT n(nullptr, 15u);
    CHECK(n.pointer() == nullptr);
    CHECK(n.tag() == 15u);
  }
  // Enumeration and bool tags.
  {
    int i = 0;
    std::pointer_tag_pair<int*, 2, Color> c(&i, Color::blue);
    CHECK(c.pointer() == &i && c.tag() == Color::blue);
    std::pointer_tag_pair<int*, 2, Plain> p(&i, p3);
    CHECK(p.pointer() == &i && p.tag() == p3);
    std::pointer_tag_pair<int*, 1, bool> b(&i, true);
    CHECK(b.pointer() == &i && b.tag() == true);
    std::pointer_tag_pair<int*, 2, unsigned char> uc(&i, static_cast<unsigned char>(3));
    CHECK(uc.pointer() == &i && uc.tag() == 3);
  }
  // Pointers to cv-qualified objects, to a base that is pointer-interconvertible, to a union,
  // and void* of a suitably aligned object.
  {
    const int ci = 1;
    std::pointer_tag_pair<const int*> pc(&ci, 3u);
    CHECK(pc.pointer() == &ci && pc.tag() == 3u);
    Derived der{};
    std::pointer_tag_pair<Base*> pb(&der, 7u);
    CHECK(pb.pointer() == static_cast<Base*>(&der) && pb.tag() == 7u);
    U u{};
    std::pointer_tag_pair<U*> pu(&u, 3u);
    CHECK(pu.pointer() == &u && pu.tag() == 3u);
    Node n{};
    std::pointer_tag_pair<void*, 4> pv(&n, 9u); // alignof(Node) = 16 gives 4 bits
    CHECK(pv.pointer() == static_cast<void*>(&n) && pv.tag() == 9u);
    std::pointer_tag_pair<const void*, 2> pcv(&ci, 2u);
    CHECK(pcv.pointer() == static_cast<const void*>(&ci) && pcv.tag() == 2u);
  }
  // [ptrtag.pair.overalign]/3: more bits than the type's alignment gives.
  {
    alignas(64) char buf[128];
    using CP = std::pointer_tag_pair<char*, 6>;
    CP a = CP::from_overaligned<64>(buf, 63u);
    CHECK(a.pointer() == buf && a.tag() == 63u);
    CP b = CP::from_overaligned<64>(buf + 64, 0u);
    CHECK(b.pointer() == buf + 64 && b.tag() == 0u);
    CP z = CP::from_overaligned<64>(static_cast<char*>(nullptr), 5u);
    CHECK(z.pointer() == nullptr && z.tag() == 5u);
    using VP = std::pointer_tag_pair<void*, 5, Color>;
    VP v = VP::from_overaligned<32>(static_cast<void*>(buf + 32), Color::green);
    CHECK(v.pointer() == static_cast<void*>(buf + 32) && v.tag() == Color::green);
    // PromisedAlignment may exceed the bits requested.
    using C2 = std::pointer_tag_pair<char*, 2>;
    C2 c2 = C2::from_overaligned<64>(buf, 3u);
    CHECK(c2.pointer() == buf && c2.tag() == 3u);
  }
  // [ptrtag.pair.tagops]/2-3: round trip through the tagged pointer, also into a DP with other
  // bits_requested, pointer type and tag type.
  {
    alignas(64) Node n{};
    PT p(&n, 5u);
    void* tp = p.tagged_pointer();
    PT q = PT::from_tagged(tp);
    CHECK(q.pointer() == &n && q.tag() == 5u);
    CHECK(q == p);
    using DP = std::pointer_tag_pair<void*, 6, unsigned>; // 6 bits: n is 64-aligned
    DP dp = DP::from_tagged(tp);
    CHECK(static_cast<Node*>(dp.pointer()) == &n);
    CHECK(static_cast<unsigned>(dp.tag()) == 5u);
    using DP3 = std::pointer_tag_pair<Node*, 3, Plain>; // the tag needs 3 bits
    DP3 dp3 = DP3::from_tagged(tp);
    CHECK(dp3.pointer() == &n && static_cast<unsigned>(dp3.tag()) == 5u);
    // A tagged pointer survives a trip through another pointer type ([ptrtag.bits]/2).
    char* as_char = static_cast<char*>(tp);
    PT r = PT::from_tagged(as_char);
    CHECK(r.pointer() == &n && r.tag() == 5u);
    // The tag 0 leaves the pointer itself.
    CHECK(PT(&n, 0u).tagged_pointer() == static_cast<void*>(&n));
    const int ci = 0;
    std::pointer_tag_pair<const int*, 2> pc(&ci, 1u);
    const void* ctp = pc.tagged_pointer();
    CHECK(std::pointer_tag_pair<const int*, 2>::from_tagged(ctp).pointer() == &ci);
  }
  // [ptrtag.pair.swap]/1, copy and assignment.
  {
    Node a{}, b{};
    PT x(&a, 1u), y(&b, 2u);
    x.swap(y);
    CHECK(x.pointer() == &b && x.tag() == 2u && y.pointer() == &a && y.tag() == 1u);
    using std::swap;
    swap(x, y);
    CHECK(x.pointer() == &a && x.tag() == 1u);
    PT c = x;
    CHECK(c == x);
    c = y;
    CHECK(c.pointer() == &b && c.tag() == 2u);
  }
  return 0;
}
