// [util.smartptr.shared.const]: shared_ptr() / shared_ptr(nullptr_t) are empty with
// use_count() == 0; explicit shared_ptr(Y* p) owns p (use_count 1), deleting it with delete
// (delete[] when T is an array type); shared_ptr(p, d) / (p, d, a) / (nullptr, d) own p
// with deleter d — d(nullptr) is called for the nullptr form; copy shares ownership
// (use_count + 1); move transfers it, leaving r empty with r.get() == nullptr; the
// converting forms require Y* compatible with T*.
#include <memory>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "test_allocators.hpp"
#include "check.hpp"

struct Counted {
  static inline int live = 0;
  int v;
  Counted(int x = 0) : v(x) { ++live; }
  Counted(const Counted& o) : v(o.v) { ++live; }
  virtual ~Counted() { --live; }
};
struct Derived : Counted {
  Derived() : Counted(7) {}
};

static int deleter_calls = 0;
static void* deleted_ptr = reinterpret_cast<void*>(1);
struct Del {
  template <class T>
  void operator()(T* p) const {
    ++deleter_calls;
    deleted_ptr = p;
    delete p;
  }
  void operator()(std::nullptr_t) const {  // for shared_ptr(nullptr_t, D): d(p) with p a nullptr_t
    ++deleter_calls;
    deleted_ptr = nullptr;
  }
};

static_assert(std::is_same_v<std::shared_ptr<int>::element_type, int>);
static_assert(std::is_same_v<std::shared_ptr<int[]>::element_type, int>);
static_assert(std::is_same_v<std::shared_ptr<int[4]>::element_type, int>);
static_assert(std::is_same_v<std::shared_ptr<int>::weak_type, std::weak_ptr<int>>);
static_assert(std::is_nothrow_default_constructible_v<std::shared_ptr<int>>);
static_assert(!std::is_convertible_v<int*, std::shared_ptr<int>>);
static_assert(std::is_constructible_v<std::shared_ptr<Counted>, Derived*>);
static_assert(!std::is_constructible_v<std::shared_ptr<Derived>, Counted*>);
static_assert(std::is_convertible_v<std::shared_ptr<Derived>, std::shared_ptr<Counted>>);
static_assert(!std::is_convertible_v<std::shared_ptr<Counted>, std::shared_ptr<Derived>>);
static_assert(std::is_constructible_v<std::shared_ptr<const int[]>, int*>);
static_assert(!std::is_constructible_v<std::shared_ptr<int[]>, const int*>);
static_assert(std::is_convertible_v<std::shared_ptr<int[3]>, std::shared_ptr<int[]>>);  // compatible
static_assert(std::is_nothrow_copy_constructible_v<std::shared_ptr<int>>);
static_assert(std::is_nothrow_move_constructible_v<std::shared_ptr<int>>);

int main() {
  {
    std::shared_ptr<int> a, b(nullptr);
    CHECK(!a && a.get() == nullptr && a.use_count() == 0);
    CHECK(!b && b.use_count() == 0);
  }
  {
    std::shared_ptr<Counted> p(new Counted(1));
    CHECK(p.use_count() == 1 && p->v == 1 && Counted::live == 1);
    std::shared_ptr<Counted> q(p);
    CHECK(p.use_count() == 2 && q.use_count() == 2 && q.get() == p.get());
    std::shared_ptr<Counted> r(std::move(q));
    CHECK(!q && q.get() == nullptr && q.use_count() == 0 && r.use_count() == 2);
    {
      std::shared_ptr<const Counted> c(r);
      CHECK(r.use_count() == 3);
    }
    CHECK(r.use_count() == 2);
  }
  CHECK(Counted::live == 0);
  {
    std::shared_ptr<Counted> base(new Derived);  // deletes through Derived* (deleter captures Y*)
    CHECK(base->v == 7);
  }
  CHECK(Counted::live == 0);
  {
    Counted* raw = new Counted(2);
    {
      std::shared_ptr<Counted> p(raw, Del{});
      std::shared_ptr<Counted> q = p;
    }
    CHECK(deleter_calls == 1 && deleted_ptr == raw);
    {
      std::shared_ptr<int> n(nullptr, Del{});
      CHECK(!n && n.use_count() == 1);  // owns the null pointer
    }
    CHECK(deleter_calls == 2 && deleted_ptr == nullptr);
    {
      alloc_counters = {};
      std::shared_ptr<Counted> a(new Counted(3), Del{}, CountingAlloc<int>());
      CHECK(alloc_counters.allocations >= 1);  // the control block uses the allocator
    }
    CHECK(deleter_calls == 3 && alloc_counters.outstanding == 0);
  }
  {
    std::shared_ptr<Counted[]> arr(new Counted[3]);
    CHECK(Counted::live == 3);
    arr[1].v = 5;
    CHECK(arr[1].v == 5 && arr.get()[1].v == 5);
  }
  CHECK(Counted::live == 0);  // delete[]
  {
    std::shared_ptr<Counted[2]> arr(new Counted[2]);
    CHECK(Counted::live == 2);
    std::shared_ptr<Counted[]> u = arr;
    CHECK(u.use_count() == 2);
  }
  CHECK(Counted::live == 0);
  return 0;
}
