// [string.require]/3: basic_string obtains its allocator as described in [container.reqmts].
// [container.reqmts]/64: copy construction uses select_on_container_copy_construction; the
// allocator is replaced by copy assignment / move assignment / swap only if the
// corresponding propagate_on_container_* is true; get_allocator() returns the most recent
// replacement. [string.cons]/30: move assignment "move assigns as a sequence container"; its
// noexcept is POCMA || is_always_equal. [container.alloc.reqmts]/28: after a = rv, a has
// the value rv had (also when allocators differ and do not propagate).
// REQUIRES: exceptions
#include <string>
#include <type_traits>
#include <utility>
#include "test_allocators.hpp"
#include "check.hpp"

template <bool C, bool M, bool S>
using Str = std::basic_string<char, std::char_traits<char>, IdAlloc<char, C, M, S>>;

static_assert(std::is_nothrow_move_assignable_v<std::string>);
static_assert(std::is_nothrow_move_assignable_v<Str<false, true, false>>);
static_assert(noexcept(std::declval<Str<false, true, false>&>().assign(std::declval<Str<false, true, false>&&>())));

const char* const big = "a string long enough that no implementation keeps it in a small buffer";

int main() {
  {  // copy assignment, propagating
    Str<true, false, false> a(big, IdAlloc<char, true, false, false>(1));
    Str<true, false, false> b("b", IdAlloc<char, true, false, false>(2));
    b = a;
    CHECK(b == big && b.get_allocator().id == 1);
  }
  {  // copy assignment, not propagating
    Str<false, false, false> a(big, IdAlloc<char>(1));
    Str<false, false, false> b("b", IdAlloc<char>(2));
    b = a;
    CHECK(b == big && b.get_allocator().id == 2);
  }
  {  // move assignment, propagating
    Str<false, true, false> a(big, IdAlloc<char, false, true, false>(1));
    Str<false, true, false> b("b", IdAlloc<char, false, true, false>(2));
    b = std::move(a);
    CHECK(b == big && b.get_allocator().id == 1);
  }
  {  // move assignment, not propagating, unequal allocators: value still transferred
    Str<false, false, false> a(big, IdAlloc<char>(1));
    Str<false, false, false> b("b", IdAlloc<char>(2));
    b = std::move(a);
    CHECK(b == big && b.get_allocator().id == 2);
    a = "usable";
    CHECK(a == "usable");
  }
  {  // move assignment, not propagating, equal allocators
    Str<false, false, false> a(big, IdAlloc<char>(3));
    Str<false, false, false> b("b", IdAlloc<char>(3));
    b = std::move(a);
    CHECK(b == big && b.get_allocator().id == 3);
  }
  {  // swap, propagating
    Str<false, false, true> a(big, IdAlloc<char, false, false, true>(1));
    Str<false, false, true> b("b", IdAlloc<char, false, false, true>(2));
    a.swap(b);
    CHECK(a == "b" && a.get_allocator().id == 2);
    CHECK(b == big && b.get_allocator().id == 1);
  }
  {  // assign(str&&) is *this = std::move(str)
    Str<false, true, false> a(big, IdAlloc<char, false, true, false>(5));
    Str<false, true, false> b;
    b.assign(std::move(a));
    CHECK(b == big && b.get_allocator().id == 5);
  }
  {  // other assignments never replace the allocator
    Str<true, true, true> b("b", IdAlloc<char, true, true, true>(9));
    b = "x";
    b.assign(big);
    b = 'c';
    b += big;
    CHECK(b.get_allocator().id == 9);
  }
  {  // self move-assignment leaves a valid object
    std::string s = big;
    std::string& r = s;
    s = std::move(r);
    s = "ok";
    CHECK(s == "ok");
  }
  return 0;
}
