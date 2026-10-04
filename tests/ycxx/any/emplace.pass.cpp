// [any.modifiers]/1-16: emplace "Calls reset(). Then direct-non-list-initializes the
// contained value of type VT ..." "Returns: A reference to the new contained value."
#include <any>
#include <initializer_list>
#include <typeinfo>
#include "check.hpp"

struct Order {
  static inline int log[4] = {};
  static inline int n = 0;
  int id;
  Order(int i) : id(i) { log[n++] = id; }
  Order(const Order& o) : id(o.id) {}
  ~Order() { log[n++] = -id; }
};
struct Sum {
  int s;
  Sum(std::initializer_list<int> il, int mul) : s(0) {
    for (int x : il) s += x * mul;
  }
};

int main() {
  {
    std::any a;
    int& r = a.emplace<int>(3);
    CHECK(a.type() == typeid(int));
    CHECK(&r == std::any_cast<int>(&a));
    r = 4;
    CHECK(std::any_cast<int>(a) == 4);
  }
  {
    std::any a(std::in_place_type<Order>, 1);
    Order::n = 0;
    Order& r = a.emplace<Order>(2);
    // reset() happens first: old value destroyed before the new one is constructed
    CHECK(Order::n >= 2);
    CHECK(Order::log[0] == -1);
    CHECK(Order::log[1] == 2);
    CHECK(r.id == 2);
    CHECK(&r == std::any_cast<Order>(&a));
  }
  {
    std::any a = 1.5;
    Sum& s = a.emplace<Sum>({1, 2, 3}, 2);
    CHECK(s.s == 12);
    CHECK(a.type() == typeid(Sum));
  }
  {
    std::any a;
    int& r = a.emplace<const int>(5);  // VT = int
    CHECK(a.type() == typeid(int));
    CHECK(r == 5);
  }
  {
    std::any a = 1;
    a.emplace<int>();
    CHECK(std::any_cast<int>(a) == 0);
  }
  return 0;
}
