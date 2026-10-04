// [any.cons]/10-21: explicit any(in_place_type_t<T>, Args&&...) and the initializer_list
// form. "Direct-non-list-initializes the contained value of type VT with
// std::forward<Args>(args)...". VT is decay_t<T>.
#include <any>
#include <initializer_list>
#include <typeinfo>
#include <utility>
#include "check.hpp"

struct Two {
  int which;
  Two(int, int) : which(1) {}
  Two(std::initializer_list<int>) : which(2) {}
};
struct ListAndArg {
  int sum, extra;
  ListAndArg(std::initializer_list<int> il, int e) : sum(0), extra(e) {
    for (int x : il) sum += x;
  }
};
struct Agg { int a, b; };
struct NoArgs { int v = 9; };

int main() {
  {
    std::any a(std::in_place_type<int>, 5);
    CHECK(a.type() == typeid(int));
    CHECK(std::any_cast<int>(a) == 5);
  }
  {
    std::any a(std::in_place_type<int>);  // value-initialised
    CHECK(std::any_cast<int>(a) == 0);
  }
  {
    std::any a(std::in_place_type<NoArgs>);
    CHECK(std::any_cast<NoArgs&>(a).v == 9);
  }
  {
    std::any a(std::in_place_type<const int>, 3);  // VT = int
    CHECK(a.type() == typeid(int));
  }
  {
    // non-list initialisation picks Two(int, int), not the initializer_list constructor
    std::any a(std::in_place_type<Two>, 1, 2);
    CHECK(std::any_cast<Two&>(a).which == 1);
  }
  {
    std::any a(std::in_place_type<Two>, {1, 2});
    CHECK(std::any_cast<Two&>(a).which == 2);
  }
  {
    std::any a(std::in_place_type<ListAndArg>, {1, 2, 3}, 4);
    CHECK(std::any_cast<ListAndArg&>(a).sum == 6);
    CHECK(std::any_cast<ListAndArg&>(a).extra == 4);
  }
  {
    // parenthesised aggregate initialisation is direct-non-list-initialisation
    std::any a(std::in_place_type<Agg>, 1, 2);
    CHECK(std::any_cast<Agg&>(a).a == 1);
    CHECK(std::any_cast<Agg&>(a).b == 2);
  }
  {
    // in_place_type_t is not stored as a value
    std::any a(std::in_place_type<long>);
    CHECK(a.type() == typeid(long));
  }
  return 0;
}
