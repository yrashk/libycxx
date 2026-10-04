// [util.smartptr.shared.const]/28-29: template<class Y, class D> shared_ptr(unique_ptr<Y, D>&&
// r): "If r.get() == nullptr, equivalent to shared_ptr(). Otherwise, if D is not a reference
// type, equivalent to shared_ptr(r.release(), std::move(r.get_deleter())). Otherwise,
// equivalent to shared_ptr(r.release(), ref(r.get_deleter()))." /1, /11: those constructors
// enable shared_from_this. Constraint: Y* compatible with T* and unique_ptr<Y, D>::pointer
// convertible to element_type*.
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Base {
  static inline int live = 0;
  Base() { ++live; }
  virtual ~Base() { --live; }
};
struct Derived : Base {};

struct Del {
  int id = 0;
  int* calls = nullptr;
  Del() = default;
  Del(int i, int* c) : id(i), calls(c) {}
  Del(Del&& o) noexcept : id(o.id), calls(o.calls) { o.id = -1; }
  Del& operator=(Del&&) = default;
  template <class T>
  void operator()(T* p) const {
    if (calls) ++*calls;
    delete p;
  }
};

struct Self : std::enable_shared_from_this<Self> {};

static_assert(std::is_constructible_v<std::shared_ptr<Base>, std::unique_ptr<Derived>&&>);
static_assert(!std::is_constructible_v<std::shared_ptr<Derived>, std::unique_ptr<Base>&&>);
static_assert(!std::is_constructible_v<std::shared_ptr<int>, std::unique_ptr<int>&>);
static_assert(std::is_convertible_v<std::unique_ptr<int>&&, std::shared_ptr<int>>);
static_assert(std::is_constructible_v<std::shared_ptr<int[]>, std::unique_ptr<int[]>&&>);

int main() {
  // null unique_ptr: empty result (use_count 0), deleter not invoked
  int calls = 0;
  {
    std::unique_ptr<int, Del> u(nullptr, Del(1, &calls));
    std::shared_ptr<int> s(std::move(u));
    CHECK(!s && s.use_count() == 0);
    CHECK(std::get_deleter<Del>(s) == nullptr);
  }
  CHECK(calls == 0);

  // value deleter is moved into the shared_ptr
  {
    std::unique_ptr<Derived, Del> u(new Derived, Del(2, &calls));
    Derived* raw = u.get();
    std::shared_ptr<Base> s(std::move(u));
    CHECK(u.get() == nullptr);
    CHECK(s.get() == raw && s.use_count() == 1);
    Del* d = std::get_deleter<Del>(s);
    CHECK(d != nullptr && d->id == 2);
    CHECK(u.get_deleter().id == -1);  // moved from
  }
  CHECK(calls == 1 && Base::live == 0);

  // reference deleter: stored as reference_wrapper to the original deleter
  calls = 0;
  Del outer(3, &calls);
  {
    std::unique_ptr<int, Del&> u(new int(4), outer);
    std::shared_ptr<int> s(std::move(u));
    CHECK(*s == 4);
    CHECK(std::get_deleter<Del>(s) == nullptr);
    auto* rw = std::get_deleter<std::reference_wrapper<Del>>(s);
    CHECK(rw != nullptr && &rw->get() == &outer);
  }
  CHECK(calls == 1);

  // default deleter, assignment form, array form
  {
    std::shared_ptr<Base> s = std::unique_ptr<Derived>(new Derived);
    CHECK(Base::live == 1 && s.use_count() == 1);
    std::shared_ptr<int[]> a(std::unique_ptr<int[]>(new int[3]{1, 2, 3}));
    CHECK(a[2] == 3);
  }
  CHECK(Base::live == 0);

  // enables shared_from_this
  {
    std::shared_ptr<Self> s(std::unique_ptr<Self>(new Self));
    CHECK(s->shared_from_this() == s);
    CHECK(s.use_count() == 1);
  }
  return 0;
}
