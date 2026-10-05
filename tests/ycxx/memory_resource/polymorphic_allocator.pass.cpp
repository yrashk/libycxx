// [mem.poly.allocator.class]: polymorphic_allocator<Tp = byte>: value_type; default
// construction uses get_default_resource(); implicit construction from memory_resource*;
// converting copy keeps the resource; not assignable. allocate(n) is resource->allocate(n *
// sizeof(Tp), alignof(Tp)) (bad_array_new_length on overflow); allocate_bytes,
// allocate_object<T>(n), new_object<T>(args...) (construct with uses-allocator construction),
// delete_object, deallocate_*; select_on_container_copy_construction() returns a default
// polymorphic_allocator (the resource is not propagated); operator== compares the resources.
// REQUIRES: exceptions
// COUNTERPART: libcxx:utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.class.general/equality.pass.cpp
#include <memory_resource>
#include <cstddef>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"
#include "recording_resource.hpp"

using std::pmr::polymorphic_allocator;
static_assert(std::is_same_v<polymorphic_allocator<>::value_type, std::byte>);
static_assert(std::is_same_v<polymorphic_allocator<int>::value_type, int>);
static_assert(std::is_convertible_v<std::pmr::memory_resource*, polymorphic_allocator<int>>);
static_assert(std::is_convertible_v<polymorphic_allocator<int>, polymorphic_allocator<double>>);
static_assert(!std::is_copy_assignable_v<polymorphic_allocator<int>>);
static_assert(std::is_nothrow_default_constructible_v<polymorphic_allocator<int>>);
static_assert(std::is_nothrow_constructible_v<polymorphic_allocator<int>, const polymorphic_allocator<char>&>);

struct alignas(32) Wide {
  char c[40];
};
struct UsesAlloc {
  using allocator_type = polymorphic_allocator<>;
  allocator_type alloc;
  int v;
  UsesAlloc(int x, const allocator_type& a) : alloc(a), v(x) {}
};

int main() {
  RecordingResource r;
  polymorphic_allocator<int> a(&r);
  CHECK(a.resource() == &r);
  int* p = a.allocate(10);
  CHECK(r.allocs == 1 && r.last_bytes == 10 * sizeof(int) && r.last_align == alignof(int));
  a.deallocate(p, 10);
  CHECK(r.deallocs == 1 && r.last_bytes == 10 * sizeof(int) && r.last_align == alignof(int));

  polymorphic_allocator<Wide> w = a;  // converting copy
  CHECK(w.resource() == &r && w == a);
  Wide* wp = w.allocate(2);
  CHECK(r.last_bytes == 2 * sizeof(Wide) && r.last_align == 32);
  w.deallocate(wp, 2);

  bool threw = false;
  try {
    (void)w.allocate(static_cast<std::size_t>(-1) / sizeof(Wide) + 1);
  } catch (const std::bad_array_new_length&) {
    threw = true;
  }
  CHECK(threw);

  polymorphic_allocator<> b(&r);
  void* raw = b.allocate_bytes(100);
  CHECK(r.last_bytes == 100 && r.last_align == alignof(std::max_align_t));
  b.deallocate_bytes(raw, 100);
  raw = b.allocate_bytes(10, 64);
  CHECK(r.last_align == 64);
  b.deallocate_bytes(raw, 10, 64);
  double* d = b.allocate_object<double>(3);
  CHECK(r.last_bytes == 3 * sizeof(double) && r.last_align == alignof(double));
  b.deallocate_object(d, 3);
  CHECK(r.last_bytes == 3 * sizeof(double));
  std::string* s = b.new_object<std::string>(3, 'x');
  CHECK(*s == "xxx");
  b.delete_object(s);
  CHECK(r.outstanding == 0);

  // construct uses uses-allocator construction.
  UsesAlloc* u = b.new_object<UsesAlloc>(7);
  CHECK(u->v == 7 && u->alloc.resource() == &r);
  b.delete_object(u);
  std::pair<UsesAlloc, int>* pr = b.allocate_object<std::pair<UsesAlloc, int>>();
  b.construct(pr, 5, 6);
  CHECK(pr->first.v == 5 && pr->first.alloc.resource() == &r && pr->second == 6);
  b.destroy(pr);
  b.deallocate_object(pr);

  // new_object deallocates if the constructor throws.
  struct Throws {
    Throws() { throw 42; }
  };
  int allocs = r.allocs, deallocs = r.deallocs;
  try {
    (void)b.new_object<Throws>();
  } catch (int) {
  }
  CHECK(r.allocs == allocs + 1 && r.deallocs == deallocs + 1);

  // select_on_container_copy_construction does not propagate the resource.
  CHECK(a.select_on_container_copy_construction().resource() == std::pmr::get_default_resource());
  polymorphic_allocator<int> def;
  CHECK(def.resource() == std::pmr::get_default_resource());

  RecordingResource other;
  CHECK(a != polymorphic_allocator<int>(&other) && a == polymorphic_allocator<char>(&r));

  // pmr containers use it.
  std::pmr::vector<int> v(&r);
  v.push_back(1);
  CHECK(v.get_allocator().resource() == &r && r.outstanding > 0);
  std::pmr::vector<std::pmr::string> vs(&r);
  vs.emplace_back("a string long enough to need a heap allocation, surely");
  CHECK(vs[0].get_allocator().resource() == &r);  // uses-allocator construction of elements
  return 0;
}
