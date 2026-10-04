// [any.cons]/5-9: template<class T> any(T&& value): "Let VT be decay_t<T>." "Constructs an
// object of type any that contains an object of type VT direct-initialized with
// std::forward<T>(value)." [any.cons]/2: copy constructor copies the contained value.
// [any.cons]/4: move constructor is noexcept.
#include <any>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Tracker {
  static inline int copies = 0, moves = 0, live = 0;
  int v;
  Tracker(int x) : v(x) { ++live; }
  Tracker(const Tracker& o) : v(o.v) { ++copies; ++live; }
  Tracker(Tracker&& o) noexcept : v(o.v) { ++moves; ++live; }
  ~Tracker() { --live; }
};

int f(int x) { return x + 1; }

static_assert(std::is_nothrow_move_constructible_v<std::any>);
static_assert(std::is_copy_constructible_v<std::any>);
static_assert(std::is_convertible_v<int, std::any>);
static_assert(std::is_convertible_v<Tracker, std::any>);

int main() {
  {
    std::any a = 42;
    CHECK(a.has_value());
    CHECK(a.type() == typeid(int));
  }
  {
    const int ci = 3;
    std::any a(ci);  // VT = int, not const int
    CHECK(a.type() == typeid(int));
  }
  {
    int arr[3] = {1, 2, 3};
    std::any a(arr);  // array decays to int*
    CHECK(a.type() == typeid(int*));
    CHECK(*std::any_cast<int*>(&a) == arr);
  }
  {
    std::any a(f);  // function decays to pointer
    CHECK(a.type() == typeid(int (*)(int)));
    CHECK((*std::any_cast<int (*)(int)>(&a))(1) == 2);
  }
  {
    std::any a = "meow";
    CHECK(a.type() == typeid(const char*));
  }
  Tracker::copies = Tracker::moves = 0;
  {
    Tracker t(7);
    std::any a(t);  // lvalue: copy
    CHECK(Tracker::copies == 1);
    CHECK(Tracker::moves == 0);
    std::any b(std::move(t));  // rvalue: move
    CHECK(Tracker::moves == 1);
    CHECK(Tracker::copies == 1);
    std::any c(a);  // copy constructor copies the contained value
    CHECK(Tracker::copies == 2);
    CHECK(c.type() == typeid(Tracker));
    CHECK(std::any_cast<Tracker&>(c).v == 7);
    CHECK(&std::any_cast<Tracker&>(c) != &std::any_cast<Tracker&>(a));
    std::any d(std::move(c));
    CHECK(d.has_value());
    CHECK(d.type() == typeid(Tracker));
    CHECK(std::any_cast<Tracker&>(d).v == 7);
    CHECK(Tracker::copies == 2);
  }
  CHECK(Tracker::live == 0);
  {
    std::any empty;
    std::any copy(empty);
    CHECK(!copy.has_value());
    std::any moved(std::move(empty));
    CHECK(!moved.has_value());
  }
  return 0;
}
