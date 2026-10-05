// [util.smartptr.weak]: weak_ptr() is empty (use_count 0, expired); constructing from a
// shared_ptr shares ownership without adding to use_count; expired() is use_count() == 0;
// lock() returns expired() ? shared_ptr<T>() : shared_ptr<T>(*this); reset() empties it;
// copy/move/convert. [util.smartptr.shared.const]/29-31: explicit shared_ptr(const
// weak_ptr<Y>&) throws bad_weak_ptr if r.expired(). [util.smartptr.weakptr]: bad_weak_ptr
// derives from exception and what() returns an implementation-defined NTBS.
// REQUIRES: exceptions
#include <memory>
#include <exception>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct B {
  virtual ~B() = default;
};
struct D : B {};

static_assert(std::is_base_of_v<std::exception, std::bad_weak_ptr>);
static_assert(std::is_nothrow_default_constructible_v<std::weak_ptr<int>>);
static_assert(std::is_convertible_v<std::weak_ptr<D>, std::weak_ptr<B>>);
static_assert(std::is_convertible_v<std::shared_ptr<D>, std::weak_ptr<B>>);
static_assert(!std::is_convertible_v<std::weak_ptr<B>, std::weak_ptr<D>>);
static_assert(!std::is_convertible_v<std::weak_ptr<int>, std::shared_ptr<int>>);  // explicit
static_assert(std::is_same_v<std::weak_ptr<int[]>::element_type, int>);

int main() {
  std::weak_ptr<int> w0;
  CHECK(w0.expired() && w0.use_count() == 0 && !w0.lock());
  std::weak_ptr<int> w;
  {
    auto s = std::make_shared<int>(5);
    w = s;
    CHECK(s.use_count() == 1 && w.use_count() == 1 && !w.expired());
    {
      auto l = w.lock();
      CHECK(l && *l == 5 && s.use_count() == 2);
    }
    std::shared_ptr<int> f(w);
    CHECK(f.get() == s.get() && s.use_count() == 2);
    std::weak_ptr<int> w2(w);
    std::weak_ptr<int> w3(std::move(w2));
    CHECK(w2.expired() && w3.use_count() == 2);
    std::weak_ptr<const int> wc(w3);
    CHECK(wc.lock().get() == s.get());
  }
  CHECK(w.expired() && w.use_count() == 0 && !w.lock());
  bool threw = false;
  try {
    std::shared_ptr<int> s(w);
  } catch (const std::bad_weak_ptr& e) {
    threw = true;
    CHECK(e.what() != nullptr);
  }
  CHECK(threw);
  {
    auto s = std::make_shared<D>();
    std::weak_ptr<B> wb(s);
    std::weak_ptr<B> wb2 = std::weak_ptr<D>(s);
    CHECK(wb.lock().get() == s.get() && wb2.lock().get() == s.get());
    wb.reset();
    CHECK(wb.expired() && !wb2.expired());
    std::weak_ptr<D> wd(s);
    swap(wb, wb2);
    CHECK(!wb.expired() && wb2.expired());
    // deduction guide shared_ptr(weak_ptr<T>) -> shared_ptr<T>
    std::shared_ptr fromw(wd);
    static_assert(std::is_same_v<decltype(fromw), std::shared_ptr<D>>);
    // weak_ptr(shared_ptr<T>) deduction
    std::weak_ptr dw(s);
    static_assert(std::is_same_v<decltype(dw), std::weak_ptr<D>>);
  }
  return 0;
}
