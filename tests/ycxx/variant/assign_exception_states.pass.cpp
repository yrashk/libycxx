// [variant.assign] -- the state after an exception, for each assignment path:
// /2.5 copy assignment when Tj's copy may throw but its move may not: "equivalent to
//   operator=(variant(rhs))", so a throwing copy happens before *this is touched: unchanged.
// /2.1-2.2, /8.1-8.2: valueless rhs -> *this becomes (or stays) valueless, no other effect.
// /10.1: "If an exception is thrown during the call to Tj's move construction (with j being
//   rhs.index()), the variant will hold no value." (required, not just permitted)
// /10.2: a throwing move assignment of the same alternative leaves index() == j.
// /13.3 converting assignment when Tj is not nothrow-constructible from T but nothrow
//   move-constructible: "emplace<j>(Tj(std::forward<T>(t)))": the temporary is built first, so
//   a throwing conversion leaves *this unchanged; /16.1: a throwing assignment to the same
//   alternative leaves valueless_by_exception() false.
// [variant.mod]/7: emplace on a valueless variant constructs without destroying anything.
// COUNTERPART: libstdcxx:20_util/variant/(87431|exception_safety).cc
// REQUIRES: exceptions
#include <variant>
#include <string>
#include <type_traits>
#include "check.hpp"

struct ThrowCopy {  // copy may throw, move is nothrow
  static inline bool armed = false;
  static inline int moves = 0, copies = 0;
  int v = 0;
  ThrowCopy(int x) : v(x) {}
  ThrowCopy(const ThrowCopy& o) : v(o.v) {
    if (armed) throw 1;
    ++copies;
  }
  ThrowCopy(ThrowCopy&& o) noexcept : v(o.v) { ++moves; }
  ThrowCopy& operator=(const ThrowCopy&) = default;
  ThrowCopy& operator=(ThrowCopy&&) = default;
};
struct ThrowMove {  // move construction and move assignment may throw
  static inline bool armed = false;
  int v = 0;
  ThrowMove(int x) : v(x) {}
  ThrowMove(const ThrowMove& o) : v(o.v) {}
  ThrowMove(ThrowMove&& o) : v(o.v) {
    if (armed) throw 2;
  }
  ThrowMove& operator=(const ThrowMove&) = default;
  ThrowMove& operator=(ThrowMove&& o) {
    if (armed) throw 5;
    v = o.v;
    return *this;
  }
};
struct ThrowConv {  // conversion from int may throw, move is nothrow
  static inline int moves = 0;
  int v = 0;
  ThrowConv(int x) noexcept(false) : v(x) {
    if (x < 0) throw 3;
  }
  ThrowConv(ThrowConv&& o) noexcept : v(o.v) { ++moves; }
  ThrowConv(const ThrowConv&) = default;
  ThrowConv& operator=(const ThrowConv&) = default;
  ThrowConv& operator=(ThrowConv&&) = default;
  ThrowConv& operator=(int x) {
    if (x == 100) throw 4;
    v = x;
    return *this;
  }
};

using VM = std::variant<int, ThrowMove>;
static VM make_valueless() {
  VM v(1), src(std::in_place_index<1>, 5);
  ThrowMove::armed = true;
  try {
    v = std::move(src);  // (8.4) emplace<1>(GET<1>(std::move(rhs))) throws in the move ctor
  } catch (int) {
  }
  ThrowMove::armed = false;
  return v;
}

int main() {
  {  // (2.5): copy first, then move; a throwing copy leaves *this unchanged.
    static_assert(!std::is_nothrow_copy_constructible_v<ThrowCopy> && std::is_nothrow_move_constructible_v<ThrowCopy>);
    std::variant<int, ThrowCopy> a = 7, b(std::in_place_index<1>, 5);
    ThrowCopy::armed = true;
    bool caught = false;
    try { a = b; } catch (int e) { caught = e == 1; }
    ThrowCopy::armed = false;
    CHECK(caught);
    CHECK(a.index() == 0 && std::get<0>(a) == 7);
    ThrowCopy::moves = ThrowCopy::copies = 0;
    a = b;
    CHECK(a.index() == 1 && std::get<1>(a).v == 5 && ThrowCopy::copies == 1 && ThrowCopy::moves == 1);
  }
  {  // (10.1): move construction of Tj throws -> valueless (required).
    VM a(7), b(std::in_place_index<1>, 5);
    ThrowMove::armed = true;
    bool caught = false;
    try { a = std::move(b); } catch (int e) { caught = e == 2; }
    ThrowMove::armed = false;
    CHECK(caught && a.valueless_by_exception() && a.index() == std::variant_npos);
    CHECK(b.index() == 1);
  }
  {  // (10.2): move assignment of the same alternative throws -> index unchanged.
    VM a(std::in_place_index<1>, 1), b(std::in_place_index<1>, 2);
    ThrowMove::armed = true;
    bool caught = false;
    try { a = std::move(b); } catch (int e) { caught = e == 5; }
    ThrowMove::armed = false;
    CHECK(caught && a.index() == 1 && !a.valueless_by_exception());
  }
  {  // valueless transitions: (2.1), (2.2), (8.1), (8.2), copy/move construction.
    VM vl = make_valueless();
    CHECK(vl.valueless_by_exception());
    VM c = vl;
    CHECK(c.valueless_by_exception());
    VM m = std::move(c);
    CHECK(m.valueless_by_exception());
    c = vl;             // (2.1) neither holds a value
    CHECK(c.valueless_by_exception());
    c = std::move(m);   // (8.1)
    CHECK(c.valueless_by_exception());
    VM d(std::in_place_index<1>, 3);
    c = d;              // valueless <- value
    CHECK(c.index() == 1 && std::get<1>(c).v == 3);
    c = vl;             // (2.2) value <- valueless
    CHECK(c.valueless_by_exception());
    c = 4;              // converting assignment into a valueless variant
    CHECK(c.index() == 0 && std::get<0>(c) == 4);
    c = make_valueless();  // (8.2)
    CHECK(c.valueless_by_exception());
    CHECK(c.emplace<1>(8).v == 8 && c.index() == 1);  // [variant.mod]/7
    VM& self = vl;
    vl = self;
    CHECK(vl.valueless_by_exception());
    vl = std::move(self);
    CHECK(vl.valueless_by_exception());
  }
  {  // (13.3): the temporary Tj is built before anything else; a throw leaves *this unchanged.
    static_assert(!std::is_nothrow_constructible_v<ThrowConv, int> && std::is_nothrow_move_constructible_v<ThrowConv>);
    std::variant<std::string, ThrowConv> a = std::string("x");
    bool caught = false;
    try { a = -1; } catch (int e) { caught = e == 3; }
    CHECK(caught);
    CHECK(a.index() == 0 && std::get<0>(a) == "x");
    ThrowConv::moves = 0;
    a = 5;
    CHECK(a.index() == 1 && std::get<1>(a).v == 5 && ThrowConv::moves == 1);
    // (13.1)/(16.1): same alternative -> assignment from the int; it throws -> still holds it.
    caught = false;
    try { a = 100; } catch (int e) { caught = e == 4; }
    CHECK(caught && a.index() == 1 && !a.valueless_by_exception() && std::get<1>(a).v == 5);
    a = 6;
    CHECK(ThrowConv::moves == 1 && std::get<1>(a).v == 6);
  }
  return 0;
}
