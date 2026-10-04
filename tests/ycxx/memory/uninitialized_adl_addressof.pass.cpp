// [specialized.algorithms.general]/4: the uninitialized algorithms obtain the storage address
// with voidify(obj) = addressof(obj) ([specialized.algorithms]: e.g. [uninitialized.copy]/1
// "::new (voidify(*result)) iter_value_t<NoThrowForwardIterator>(*first)"), so a deleted unary
// operator& on the element type is never used; [specialized.destroy]: destroy(first, last) is
// "for (; first != last; ++first) destroy_at(addressof(*first))". [contents]/3: unqualified
// names in these specifications are not looked up by ADL, so unconstrained function templates
// named construct_at, destroy_at, addressof, move, ... in the element's namespace
// (support/adl_poison.hpp) are not found. The iterators (evil::Iter) have a deleted comma
// operator, which the iterator requirements do not include. Both the std and the ranges forms.
#include <memory>
#include <cstddef>
#include <new>
#include "adl_poison.hpp"
#include "check.hpp"

using evil::Iter;
using evil::Val;

alignas(Val) unsigned char raw[8 * sizeof(Val)];

Val* storage() { return reinterpret_cast<Val*>(raw); }

bool all_equal(int v, int n) {
  for (int i = 0; i < n; ++i)
    if (storage()[i].v != v) return false;
  return true;
}

int main() {
  Val src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  Iter<Val> sb(src), se(src + 8);
  Iter<Val> d(storage()), de(storage() + 8);

  CHECK(std::uninitialized_copy(sb, se, d) == de);
  CHECK(storage()[7].v == 8);
  std::destroy(d, de);
  CHECK(std::uninitialized_copy_n(sb, 8, d) == de);
  CHECK(std::destroy_n(d, 8) == de);
  CHECK(std::uninitialized_move(sb, se, d) == de);
  std::destroy(d, de);
  CHECK(std::uninitialized_move_n(sb, 8, d).second == de);
  std::destroy(d, de);
  std::uninitialized_fill(d, de, Val(4));
  CHECK(all_equal(4, 8));
  std::destroy(d, de);
  CHECK(std::uninitialized_fill_n(d, 8, Val(5)) == de);
  CHECK(all_equal(5, 8));
  std::destroy(d, de);
  std::uninitialized_value_construct(d, de);
  CHECK(all_equal(0, 8));
  std::destroy(d, de);
  CHECK(std::uninitialized_value_construct_n(d, 8) == de);
  std::destroy(d, de);
  std::uninitialized_default_construct(d, de);
  std::destroy(d, de);
  CHECK(std::uninitialized_default_construct_n(d, 8) == de);
  std::destroy(d, de);
  Val* p = std::construct_at(storage(), 9);
  CHECK(p->v == 9);
  std::destroy_at(p);

  namespace r = std::ranges;
  CHECK(r::uninitialized_copy(sb, se, d, de).out == de);
  CHECK(r::destroy(d, de) == de);
  CHECK(r::uninitialized_copy_n(sb, 8, d, de).out == de);
  CHECK(r::destroy_n(d, 8) == de);
  CHECK(r::uninitialized_move(sb, se, d, de).out == de);
  r::destroy(d, de);
  CHECK(r::uninitialized_move_n(sb, 8, d, de).out == de);
  r::destroy(d, de);
  CHECK(r::uninitialized_fill(d, de, Val(6)) == de);
  CHECK(all_equal(6, 8));
  r::destroy(d, de);
  CHECK(r::uninitialized_fill_n(d, 8, Val(7)) == de);
  r::destroy(d, de);
  CHECK(r::uninitialized_value_construct(d, de) == de);
  CHECK(all_equal(0, 8));
  r::destroy(d, de);
  CHECK(r::uninitialized_value_construct_n(d, 8) == de);
  r::destroy(d, de);
  CHECK(r::uninitialized_default_construct(d, de) == de);
  r::destroy(d, de);
  CHECK(r::uninitialized_default_construct_n(d, 8) == de);
  r::destroy(d, de);
  p = r::construct_at(storage(), 10);
  CHECK(p->v == 10);
  r::destroy_at(p);

  // addressof itself on the element type ([specialized.addressof])
  Val v(3);
  CHECK(&std::addressof(v)->v == &v.v);
  return 0;
}
