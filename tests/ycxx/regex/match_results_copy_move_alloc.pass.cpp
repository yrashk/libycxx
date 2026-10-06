// [re.results.const]/2-3: match_results(const Allocator&) stores the allocator, is not ready()
// and has size() 0. /4-10 and Table 122: the copy and move constructors (with and without an
// allocator) and assignments reproduce ready(), size(), str(n), prefix(), suffix(), [n],
// length(n) and position(n) (for moves, the values the source had); /4: the allocator-extended
// forms store the given allocator, the copy constructor gets select_on_container_copy_construction,
// /6: the move constructor moves the source's allocator and is noexcept.
// [re.results.acc]/4: position(n) is the distance from the start of the target sequence, which
// a copy keeps. /8: [n] for n >= size() is an unmatched sub_match.
// [re.results.swap]/1-4: member and non-member swap exchange the results.
#include <regex>
#include <string>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "test_allocators.hpp"

using Alloc = IdAlloc<std::csub_match>;
using M = std::match_results<const char*, Alloc>;

static_assert(std::is_nothrow_move_constructible_v<M>);
static_assert(!std::is_convertible_v<const Alloc&, M>); // explicit

const char text[] = "key=value; other";

void check_same(const M& a, const M& b) {
  CHECK(a.ready() == b.ready() && a.size() == b.size());
  if (!b.ready()) return;
  CHECK(a.prefix() == b.prefix() && a.suffix() == b.suffix());
  CHECK(a.prefix().first == b.prefix().first && a.suffix().second == b.suffix().second);
  for (std::size_t n = 0; n < b.size(); ++n) {
    CHECK(a.str(n) == b.str(n) && a.length(n) == b.length(n) && a.position(n) == b.position(n));
    CHECK(a[n].first == b[n].first && a[n].second == b[n].second && a[n].matched == b[n].matched);
  }
}

int main() {
  // /2-3
  M empty{Alloc(7)};
  CHECK(!empty.ready() && empty.size() == 0 && empty.empty() && empty.get_allocator().id == 7);

  M m{Alloc(3)};
  CHECK(std::regex_search(text, m, std::regex("(\\w+)=(\\w+)(x)?")));
  CHECK(m.ready() && m.size() == 4 && m.str(1) == "key" && m.position(2) == 4 && !m[3].matched);
  CHECK(m.prefix().length() == 0 && !m.prefix().matched && m.suffix() == "; other");
  // [re.results.acc]/8
  CHECK(!m[4].matched && m[4].length() == 0 && !m[100].matched);

  // Copy: Table 122, and the allocator from select_on_container_copy_construction (IdAlloc's
  // default is a copy).
  M c(m);
  check_same(c, m);
  CHECK(c.get_allocator().id == 3);
  // The sub_matches of the copy still point into the original target sequence.
  CHECK(c[1].first == text && c.position(2) == 4);

  // Copy with an allocator.
  M ca(m, Alloc(9));
  check_same(ca, m);
  CHECK(ca.get_allocator().id == 9);

  // Copy of a not-ready object.
  M ce(empty);
  CHECK(!ce.ready() && ce.size() == 0);

  // Move: the postconditions name the values m had before.
  M keep(m);
  M mv(std::move(m));
  check_same(mv, keep);
  CHECK(mv.get_allocator().id == 3);

  M keep2(mv);
  M mva(std::move(mv), Alloc(3)); // equal allocator
  check_same(mva, keep2);
  M mvb(std::move(mva), Alloc(5)); // unequal allocator: still the same values
  check_same(mvb, keep2);
  CHECK(mvb.get_allocator().id == 5);

  // Copy and move assignment.
  M a{Alloc(1)};
  a = keep;
  check_same(a, keep);
  M b{Alloc(1)};
  b = std::move(a);
  check_same(b, keep);
  M nr{Alloc(1)};
  b = nr; // assigning a not-ready one
  CHECK(!b.ready() && b.size() == 0);

  // [re.results.swap]
  M s1{Alloc(0)}, s2{Alloc(0)};
  CHECK(std::regex_match("ab", s1, std::regex("(a)(b)")));
  s1.swap(s2);
  CHECK(!s1.ready() && s2.ready() && s2.size() == 3 && s2.str(2) == "b");
  swap(s1, s2);
  CHECK(s1.ready() && s1.str(1) == "a" && !s2.ready());
  std::cmatch g1, g2;
  CHECK(std::regex_search("xyz", g1, std::regex("y")));
  std::swap(g1, g2);
  CHECK(g2.ready() && g2.position() == 1 && !g1.ready());
  return 0;
}
