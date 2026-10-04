// [any.assign]/1: copy assignment: "No effects if an exception is thrown."
// [any.assign]/10: operator=(T&&): "No effects if an exception is thrown."
// [any.modifiers]/8: emplace: "If an exception is thrown during the call to VT's
// constructor, *this does not contain a value, and any previously contained value has
// been destroyed."
#include <any>
#include <initializer_list>
#include <typeinfo>
#include "check.hpp"

struct Boom {};
struct ThrowsOnCopy {
  static inline bool armed = false;
  ThrowsOnCopy() = default;
  ThrowsOnCopy(std::initializer_list<int>) { throw Boom{}; }
  ThrowsOnCopy(int) { throw Boom{}; }
  ThrowsOnCopy(const ThrowsOnCopy&) {
    if (armed) throw Boom{};
  }
};
struct Watch {
  static inline int live = 0;
  int v;
  Watch(int x) : v(x) { ++live; }
  Watch(const Watch& o) : v(o.v) { ++live; }
  ~Watch() { --live; }
};

int main() {
  {
    ThrowsOnCopy::armed = false;
    std::any src(std::in_place_type<ThrowsOnCopy>);
    std::any dst = Watch(5);
    ThrowsOnCopy::armed = true;
    bool caught = false;
    try {
      dst = src;
    } catch (Boom) {
      caught = true;
    }
    CHECK(caught);
    CHECK(dst.type() == typeid(Watch));
    CHECK(std::any_cast<Watch&>(dst).v == 5);
  }
  {
    ThrowsOnCopy::armed = true;
    ThrowsOnCopy t;
    std::any dst = Watch(6);
    bool caught = false;
    try {
      dst = t;
    } catch (Boom) {
      caught = true;
    }
    CHECK(caught);
    CHECK(dst.type() == typeid(Watch));
    CHECK(std::any_cast<Watch&>(dst).v == 6);
  }
  CHECK(Watch::live == 0);
  {
    std::any dst = Watch(7);
    bool caught = false;
    try {
      dst.emplace<ThrowsOnCopy>(1);
    } catch (Boom) {
      caught = true;
    }
    CHECK(caught);
    CHECK(!dst.has_value());
    CHECK(Watch::live == 0);
  }
  {
    std::any dst = Watch(8);
    bool caught = false;
    try {
      dst.emplace<ThrowsOnCopy>({1, 2});
    } catch (Boom) {
      caught = true;
    }
    CHECK(caught);
    CHECK(!dst.has_value());
    CHECK(Watch::live == 0);
  }
  {
    // constructor throwing propagates the exception out of any(T&&) / in_place constructors
    bool caught = false;
    try {
      std::any a(std::in_place_type<ThrowsOnCopy>, 3);
    } catch (Boom) {
      caught = true;
    }
    CHECK(caught);
  }
  return 0;
}
