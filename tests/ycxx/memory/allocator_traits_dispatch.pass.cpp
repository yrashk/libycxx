// [allocator.traits.members]: allocate(a, n) returns a.allocate(n); allocate(a, n, hint)
// returns "a.allocate(n, hint) if that expression is well-formed; otherwise, a.allocate(n)";
// deallocate calls a.deallocate(p, n); construct "Calls a.construct(p,
// std::forward<Args>(args)...) if that call is well-formed; otherwise, invokes construct_at(p,
// std::forward<Args>(args)...)"; destroy "Calls a.destroy(p) if that call is well-formed;
// otherwise, invokes destroy_at(p)"; max_size "Returns: a.max_size() if that expression is
// well-formed; otherwise, numeric_limits<size_type>::max() / sizeof(value_type)";
// select_on_container_copy_construction "Returns: rhs.select_on_container_copy_construction()
// if that expression is well-formed; otherwise, rhs." All are constexpr.
#include <memory>
#include <cstddef>
#include <limits>
#include <new>
#include <utility>
#include "check.hpp"

struct Log {
  int alloc = 0, alloc_hint = 0, dealloc = 0, construct = 0, destroy = 0;
};

template <class T>
struct Full {
  using value_type = T;
  Log* log;
  int id = 0;
  explicit Full(Log* l, int i = 0) : log(l), id(i) {}
  T* allocate(std::size_t n) {
    ++log->alloc;
    return static_cast<T*>(::operator new(n * sizeof(T)));
  }
  T* allocate(std::size_t n, const void*) {
    ++log->alloc_hint;
    return static_cast<T*>(::operator new(n * sizeof(T)));
  }
  void deallocate(T* p, std::size_t) {
    ++log->dealloc;
    ::operator delete(p);
  }
  template <class U, class... A>
  void construct(U* p, A&&... a) {
    ++log->construct;
    ::new (static_cast<void*>(p)) U(std::forward<A>(a)..., 100);  // marks the value
  }
  template <class U>
  void destroy(U* p) {
    ++log->destroy;
    p->~U();
  }
  std::size_t max_size() const { return 17; }
  Full select_on_container_copy_construction() const { return Full(log, id + 1); }
  friend bool operator==(const Full&, const Full&) = default;
};

template <class T>
struct Minimal {
  using value_type = T;
  Log* log;
  int id = 0;
  explicit Minimal(Log* l, int i = 0) : log(l), id(i) {}
  T* allocate(std::size_t n) {
    ++log->alloc;
    return static_cast<T*>(::operator new(n * sizeof(T)));
  }
  void deallocate(T* p, std::size_t) {
    ++log->dealloc;
    ::operator delete(p);
  }
  friend bool operator==(const Minimal&, const Minimal&) = default;
};

struct Elem {
  static inline int dtors = 0;
  int a, b;
  Elem(int x, int y) : a(x), b(y) {}
  explicit Elem(int x) : a(x), b(-1) {}
  ~Elem() { ++dtors; }
};

constexpr bool constexpr_default_path() {
  std::allocator<int> a;
  using T = std::allocator_traits<std::allocator<int>>;
  int* p = T::allocate(a, 2);
  T::construct(a, p, 5);
  T::construct(a, p + 1);
  bool ok = p[0] == 5 && p[1] == 0;
  T::destroy(a, p);
  T::destroy(a, p + 1);
  T::deallocate(a, p, 2);
  return ok;
}
static_assert(constexpr_default_path());

int main() {
  Log log;
  {
    using T = std::allocator_traits<Full<Elem>>;
    Full<Elem> a(&log, 1);
    Elem* p = T::allocate(a, 2);
    CHECK(log.alloc == 1);
    Elem* q = T::allocate(a, 1, p);
    CHECK(log.alloc_hint == 1 && log.alloc == 1);
    T::construct(a, p, 7);  // a.construct appends 100: Elem(7, 100)
    CHECK(log.construct == 1 && p->a == 7 && p->b == 100);
    Elem::dtors = 0;
    T::destroy(a, p);
    CHECK(log.destroy == 1 && Elem::dtors == 1);
    T::deallocate(a, p, 2);
    T::deallocate(a, q, 1);
    CHECK(log.dealloc == 2);
    CHECK(T::max_size(a) == 17);
    Full<Elem> c = T::select_on_container_copy_construction(a);
    CHECK(c.id == 2);
  }
  log = Log{};
  {
    using T = std::allocator_traits<Minimal<Elem>>;
    Minimal<Elem> a(&log, 5);
    Elem* p = T::allocate(a, 1);
    Elem* q = T::allocate(a, 1, p);  // no hint overload: falls back to allocate(n)
    CHECK(log.alloc == 2);
    T::construct(a, p, 3, 4);  // construct_at
    CHECK(p->a == 3 && p->b == 4);
    T::construct(a, q, 8);  // construct_at uses direct-initialization: explicit ctor is fine
    CHECK(q->a == 8 && q->b == -1);
    Elem::dtors = 0;
    T::destroy(a, p);
    T::destroy(a, q);
    CHECK(Elem::dtors == 2);
    T::deallocate(a, p, 1);
    T::deallocate(a, q, 1);
    CHECK(log.dealloc == 2);
    CHECK(T::max_size(a) == std::numeric_limits<std::size_t>::max() / sizeof(Elem));
    Minimal<Elem> c = T::select_on_container_copy_construction(a);
    CHECK(c.id == 5 && c.log == &log);
  }
  return 0;
}
