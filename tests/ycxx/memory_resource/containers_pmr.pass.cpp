// [mem.res.syn] and the container synopses: std::pmr aliases such as pmr::string, pmr::vector,
// pmr::list, pmr::map, pmr::unordered_map, pmr::deque use polymorphic_allocator; copying a
// pmr container uses select_on_container_copy_construction (the default resource), while
// the allocator-extended copy constructor uses the given resource; polymorphic_allocator does
// not propagate on assignment or swap, so assignment keeps each container's resource.
// REQUIRES: exceptions
#include <memory_resource>
#include <deque>
#include <list>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include "check.hpp"
#include "recording_resource.hpp"

static_assert(std::is_same_v<std::pmr::string, std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>>);
static_assert(std::is_same_v<std::pmr::vector<int>, std::vector<int, std::pmr::polymorphic_allocator<int>>>);
static_assert(std::is_same_v<std::pmr::map<int, int>::allocator_type, std::pmr::polymorphic_allocator<std::pair<const int, int>>>);
using PA = std::allocator_traits<std::pmr::polymorphic_allocator<int>>;
static_assert(!PA::propagate_on_container_copy_assignment::value);
static_assert(!PA::propagate_on_container_move_assignment::value);
static_assert(!PA::propagate_on_container_swap::value);
static_assert(!PA::is_always_equal::value);

int main() {
  RecordingResource r1, r2;
  std::pmr::vector<int> a({1, 2, 3}, &r1);
  std::pmr::vector<int> copy(a);
  CHECK(copy.get_allocator().resource() == std::pmr::get_default_resource());
  std::pmr::vector<int> ext(a, &r2);
  CHECK(ext.get_allocator().resource() == &r2 && ext == a);
  std::pmr::vector<int> b(&r2);
  b = a;
  CHECK(b.get_allocator().resource() == &r2 && b == a);
  b = std::move(a);
  CHECK(b.get_allocator().resource() == &r2 && b.size() == 3);

  std::pmr::list<int> l({1, 2}, &r1);
  std::pmr::deque<int> d(5, 0, &r1);
  std::pmr::map<int, std::pmr::string> m(&r1);
  m[1] = "a fairly long string value that needs dynamic memory";
  CHECK(m[1].get_allocator().resource() == &r1);
  std::pmr::unordered_map<int, int> um(&r2);
  um[3] = 4;
  CHECK(r1.allocs > 0 && r2.allocs > 0);
  return 0;
}
