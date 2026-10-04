// [mem.poly.allocator.mem]/14-15: polymorphic_allocator::construct(p, args...) "Constructs a
// T object in the storage whose address is represented by p by uses-allocator construction
// with allocator *this and constructor arguments std::forward<Args>(args)..."
// ([allocator.uses.construction]; for pair, each member separately, /5-6 of
// uses_allocator_construction_args). Containers construct their elements through it
// ([container.alloc.reqmts]/2 Note 2), so the elements of a std::pmr::vector / deque / list /
// forward_list of pmr::string, of pair<pmr::string, int> and of pmr::vector<int> use the
// container's memory resource whichever way they are inserted (emplace from char data,
// copy or move of a string using another resource, resize, growth); the elements of a copy
// use the copy's resource: [mem.poly.allocator.mem]/19 select_on_container_copy_construction
// returns polymorphic_allocator() (the default resource) for the copy constructor, and the
// allocator-extended copy constructor uses the given one ([container.alloc.reqmts]/13-14).
// [mem.res.syn], [vector.syn], [deque.syn], [list.syn], [forward.list.syn], [string.syn]:
// the pmr aliases use polymorphic_allocator.
#include <memory_resource>
#include <deque>
#include <forward_list>
#include <list>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "move_only_elem.hpp"
#include "recording_resource.hpp"
#include "check.hpp"

using S = std::pmr::string;
const char* const long_text = "a string much longer than any small-string buffer, 01234567890123456789";

template <class C, class Get>
bool all_use(const C& c, std::pmr::memory_resource* r, Get get) {
  for (const auto& e : c)
    if (get(e).get_allocator().resource() != r) return false;
  return true;
}

template <template <class> class C>
void test() {
  RecordingResource r1, r2, rd;
  std::pmr::memory_resource* old_default = std::pmr::set_default_resource(&rd);
  auto self = [](const auto& e) -> const auto& { return e; };
  {
    C<S> c(&r1);
    append_to(c, long_text);
    S other(long_text, &r2);
    append_to(c, other);
    append_to(c, std::move(other));
    for (int i = 0; i < 40; ++i) append_to(c, long_text);  // growth / relocation
    CHECK(all_use(c, &r1, self));
    CHECK(r1.allocs > 40);
    if constexpr (requires { c.resize(1); }) {
      c.resize(60);
      CHECK(all_use(c, &r1, self));
    }
    C<S> copy(c);
    CHECK(copy.get_allocator().resource() == &rd);
    CHECK(all_use(copy, &rd, self));
    C<S> copy2(c, &r2);
    CHECK(copy2.get_allocator().resource() == &r2);
    CHECK(all_use(copy2, &r2, self));
    C<S> moved(std::move(copy2), &r1);  // unequal: element-wise, into r1
    CHECK(all_use(moved, &r1, self));
    copy = c;  // no propagation: copy keeps rd
    CHECK(all_use(copy, &rd, self));
  }
  CHECK(r1.outstanding == 0 && r2.outstanding == 0 && rd.outstanding == 0);
  {
    using P = std::pair<S, int>;
    C<P> c(&r1);
    append_to(c, long_text, 1);
    append_to(c, std::piecewise_construct, std::forward_as_tuple(long_text), std::forward_as_tuple(2));
    P outside(S(long_text, &r2), 3);
    append_to(c, outside);
    append_to(c, std::move(outside));
    CHECK(all_use(c, &r1, [](const P& p) -> const S& { return p.first; }));
  }
  {
    using V = std::pmr::vector<int>;
    C<V> c(&r1);
    append_to(c, 10, 5);  // V(10, 5, alloc)
    V outside({1, 2, 3}, &r2);
    append_to(c, outside);
    CHECK(all_use(c, &r1, self));
    CHECK(c.begin()->size() == 10 && (*c.begin())[9] == 5);
  }
  CHECK(r1.outstanding == 0 && r2.outstanding == 0);
  std::pmr::set_default_resource(old_default);
}

template <class T>
using Vec = std::pmr::vector<T>;
template <class T>
using Deq = std::pmr::deque<T>;
template <class T>
using Lst = std::pmr::list<T>;
template <class T>
using Fwd = std::pmr::forward_list<T>;

int main() {
  test<Vec>();
  test<Deq>();
  test<Lst>();
  test<Fwd>();
  return 0;
}
