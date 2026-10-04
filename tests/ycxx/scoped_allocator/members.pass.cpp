// [allocator.adaptor.cnstr], [allocator.adaptor.members], [scoped.adaptor.operators]: the
// constructor from allocators initializes outer and inner allocators recursively;
// inner_allocator() is *this with no inner allocators, else the inner adaptor;
// outer_allocator() is the OuterAlloc base; allocate/deallocate/max_size forward to the outer
// allocator; select_on_container_copy_construction applies to each allocator; == compares
// outer and (if any) inner allocators.
#include <scoped_allocator>
#include <memory>
#include <type_traits>
#include "check.hpp"
#include "test_allocators.hpp"

struct SelectAlloc : IdAlloc<int> {
  using IdAlloc<int>::IdAlloc;
  template <class U> struct rebind { using other = SelectAlloc; };
  SelectAlloc select_on_container_copy_construction() const { return SelectAlloc(id + 100); }
};

int main() {
  using S2 = std::scoped_allocator_adaptor<IdAlloc<int>, IdAlloc<char>>;
  S2 s(IdAlloc<int>(1), IdAlloc<char>(2));
  CHECK(s.outer_allocator().id == 1 && s.inner_allocator().outer_allocator().id == 2);
  CHECK(&s.outer_allocator() == static_cast<IdAlloc<int>*>(&s));
  static_assert(std::is_same_v<decltype(s.inner_allocator()), std::scoped_allocator_adaptor<IdAlloc<char>>&>);
  static_assert(noexcept(s.inner_allocator()) && noexcept(s.outer_allocator()));
  const S2& cs = s;
  static_assert(std::is_same_v<decltype(cs.outer_allocator()), const IdAlloc<int>&>);

  using S1 = std::scoped_allocator_adaptor<IdAlloc<int>>;
  S1 one(IdAlloc<int>(5));
  CHECK(&one.inner_allocator() == &one);

  S2 def;
  CHECK(def.outer_allocator().id == 0 && def.inner_allocator().outer_allocator().id == 0);

  int* p = s.allocate(3);
  s.deallocate(p, 3);
  p = s.allocate(2, nullptr);
  s.deallocate(p, 2);
  CHECK(s.max_size() == std::allocator_traits<IdAlloc<int>>::max_size(IdAlloc<int>(1)));

  // Equality.
  CHECK(s == S2(IdAlloc<int>(1), IdAlloc<char>(2)));
  CHECK(s != S2(IdAlloc<int>(1), IdAlloc<char>(3)));
  CHECK(s != S2(IdAlloc<int>(4), IdAlloc<char>(2)));
  CHECK(one == S1(IdAlloc<int>(5)) && one != S1(IdAlloc<int>(6)));
  std::scoped_allocator_adaptor<IdAlloc<double>, IdAlloc<char>> other(IdAlloc<double>(1), IdAlloc<char>(2));
  CHECK(s == other);

  // Converting construction keeps every allocator.
  S2 conv(other);
  CHECK(conv.outer_allocator().id == 1 && conv.inner_allocator().outer_allocator().id == 2);

  // select_on_container_copy_construction recurses.
  std::scoped_allocator_adaptor<SelectAlloc, SelectAlloc> sel(SelectAlloc(1), SelectAlloc(2));
  auto c = sel.select_on_container_copy_construction();
  CHECK(c.outer_allocator().id == 101 && c.inner_allocator().outer_allocator().id == 102);
  return 0;
}
