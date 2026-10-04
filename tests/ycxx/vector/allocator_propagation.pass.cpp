// [container.reqmts]/64: copy constructors obtain the allocator via
// select_on_container_copy_construction; move constructors move-construct it; copy / move
// assignment and swap replace it only if the corresponding propagate_on_container_* is
// true; get_allocator() returns the current allocator. [container.alloc.reqmts]/21-29:
// after a = t, a == t; after a = rv, a has rv's former value even with unequal,
// non-propagating allocators (elements are then move assigned or move constructed).
// [vector.overview]: move assignment is noexcept(POCMA || is_always_equal).
#include <algorithm>
#include <vector>
#include <string>
#include <type_traits>
#include <utility>
#include "test_allocators.hpp"
#include "check.hpp"

template <class T>
struct SoccAlloc {
  using value_type = T;
  int id = 0;
  SoccAlloc() = default;
  explicit SoccAlloc(int i) : id(i) {}
  template <class U>
  SoccAlloc(const SoccAlloc<U>& o) : id(o.id) {}
  SoccAlloc select_on_container_copy_construction() const { return SoccAlloc(id + 100); }
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  friend bool operator==(const SoccAlloc& a, const SoccAlloc& b) { return a.id == b.id; }
};

template <bool C, bool M, bool S>
using V = std::vector<std::string, IdAlloc<std::string, C, M, S>>;
template <bool C, bool M, bool S>
using A = IdAlloc<std::string, C, M, S>;

static_assert(std::is_nothrow_move_assignable_v<std::vector<int>>);
static_assert(std::is_nothrow_move_assignable_v<V<false, true, false>>);

const std::vector<std::string> values{"one", "two", "a third value long enough for the heap"};

int main() {
  {
    std::vector<int, SoccAlloc<int>> a({1, 2}, SoccAlloc<int>(1));
    std::vector<int, SoccAlloc<int>> b(a);
    CHECK(b == a && b.get_allocator().id == 101);
    std::vector<int, SoccAlloc<int>> c(std::move(a));
    CHECK(c.get_allocator().id == 1 && c.size() == 2);
  }
  {
    V<true, false, false> a(values.begin(), values.end(), A<true, false, false>(1));
    V<true, false, false> b(A<true, false, false>(2));
    b = a;
    CHECK(b.get_allocator().id == 1 && std::equal(b.begin(), b.end(), values.begin(), values.end()));
  }
  {
    V<false, false, false> a(values.begin(), values.end(), A<false, false, false>(1));
    V<false, false, false> b({"x"}, A<false, false, false>(2));
    b = a;
    CHECK(b.get_allocator().id == 2 && b.size() == 3 && b[2] == values[2]);
  }
  {
    V<false, true, false> a(values.begin(), values.end(), A<false, true, false>(1));
    V<false, true, false> b(A<false, true, false>(2));
    const std::string* p = a.data();
    b = std::move(a);
    CHECK(b.get_allocator().id == 1 && b.size() == 3 && b.data() == p);  // buffer transferred
  }
  {
    // Unequal, non-propagating: element-wise transfer into b's own storage.
    V<false, false, false> a(values.begin(), values.end(), A<false, false, false>(1));
    V<false, false, false> b({"old0", "old1", "old2", "old3", "old4"}, A<false, false, false>(2));
    b = std::move(a);
    CHECK(b.get_allocator().id == 2 && b.size() == 3);
    CHECK(b[0] == "one" && b[1] == "two" && b[2] == values[2]);
    a.assign(2, "reused");  // moved-from vector stays usable
    CHECK(a.size() == 2 && a[1] == "reused" && a.get_allocator().id == 1);
  }
  {
    V<false, false, false> a(values.begin(), values.end(), A<false, false, false>(3));
    V<false, false, false> b(A<false, false, false>(3));
    b = std::move(a);
    CHECK(b.size() == 3 && b[1] == "two");
  }
  {
    V<false, false, true> a({"a"}, A<false, false, true>(1));
    V<false, false, true> b({"b", "c"}, A<false, false, true>(2));
    a.swap(b);
    CHECK(a.get_allocator().id == 2 && b.get_allocator().id == 1 && a.size() == 2);
  }
  {
    V<true, true, true> a(A<true, true, true>(4));
    a = {"x", "y"};
    a.assign(3, "z");
    a.assign(values.begin(), values.end());
    a.insert(a.begin(), "w");
    CHECK(a.get_allocator().id == 4 && a.size() == 4);
  }
  return 0;
}
