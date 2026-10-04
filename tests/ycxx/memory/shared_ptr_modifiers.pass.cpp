// [util.smartptr.shared.assign]: copy assignment is shared_ptr(r).swap(*this), move
// assignment shared_ptr(std::move(r)).swap(*this), assignment from unique_ptr&&
// shared_ptr(std::move(r)).swap(*this); all return *this. [util.smartptr.shared.mod]:
// swap exchanges; reset() is shared_ptr().swap(*this); reset(p), reset(p, d), reset(p, d,
// a) are shared_ptr(p, ...).swap(*this). [util.smartptr.shared.spec]: non-member swap.
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Counted {
  static inline int live = 0;
  int v;
  Counted(int x) : v(x) { ++live; }
  ~Counted() { --live; }
};

static int del_calls = 0;
struct Del {
  void operator()(Counted* p) const {
    ++del_calls;
    delete p;
  }
};

int main() {
  {
    auto a = std::make_shared<Counted>(1);
    auto b = std::make_shared<Counted>(2);
    auto& r = (a = b);
    CHECK(&r == &a && a.get() == b.get() && b.use_count() == 2 && Counted::live == 1);
    auto c = std::make_shared<Counted>(3);
    a = std::move(c);
    CHECK(!c && a->v == 3 && b.use_count() == 1);
    std::shared_ptr<const Counted> cc;
    cc = a;  // converting copy assignment
    CHECK(cc.use_count() == 2);
    cc = std::move(b);  // converting move assignment
    CHECK(!b && cc->v == 2 && a.use_count() == 1);
    std::unique_ptr<Counted, Del> u(new Counted(4));
    a = std::move(u);
    CHECK(!u && a->v == 4 && Counted::live == 2);
    a.reset();
    CHECK(del_calls == 1 && !a && a.use_count() == 0);
    a.reset(new Counted(5));
    CHECK(a->v == 5 && a.use_count() == 1);
    a.reset(new Counted(6), Del{});
    CHECK(a->v == 6);
    a.reset(new Counted(7), Del{}, std::allocator<int>());
    CHECK(del_calls == 2 && a->v == 7);
    std::shared_ptr<Counted> s = std::make_shared<Counted>(8);
    a.swap(s);
    CHECK(a->v == 8 && s->v == 7);
    swap(a, s);
    CHECK(a->v == 7);
    std::swap(a, s);
    CHECK(a->v == 8);
    static_assert(noexcept(a.swap(s)));
    static_assert(noexcept(a.reset()));
  }
  CHECK(Counted::live == 0 && del_calls == 3);
  {
    // unique_ptr -> shared_ptr: an empty unique_ptr gives an empty shared_ptr.
    std::unique_ptr<int> e;
    std::shared_ptr<int> s(std::move(e));
    CHECK(!s && s.use_count() == 0);
    std::shared_ptr<int> t(std::make_unique<int>(3));
    CHECK(*t == 3 && t.use_count() == 1);
    // deduction guide: shared_ptr(unique_ptr<T, D>) -> shared_ptr<T>
    std::shared_ptr d(std::make_unique<long>(4));
    static_assert(std::is_same_v<decltype(d), std::shared_ptr<long>>);
    // unique_ptr<T[]> -> shared_ptr<T[]> uses delete[]
    std::shared_ptr<int[]> arr(std::make_unique<int[]>(3));
    CHECK(arr[2] == 0);
  }
  return 0;
}
