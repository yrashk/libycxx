// [type.info]: operator== "Returns: true if the two values describe the same type."
// before(): "true if *this precedes rhs in the implementation's collation order" -- an order,
// so irreflexive, asymmetric and transitive, and equal types are not ordered against each
// other. hash_code(): "within a single execution of the program, it shall return the same
// value for any two type_info objects which compare equal." name(): "an implementation-defined
// ntbs".
#include <cstddef>
#include <typeinfo>
#include "check.hpp"

struct A {
  virtual ~A() = default;
};
struct B : A {};
template <class T>
struct Tmpl {};
namespace ns {
struct A {};
}  // namespace ns

int main() {
  B b;
  A& ar = b;
  const std::type_info* t[] = {
      &typeid(int),     &typeid(long),         &typeid(A),        &typeid(B),
      &typeid(ns::A),   &typeid(Tmpl<int>),    &typeid(Tmpl<A>),  &typeid(int*),
      &typeid(void),    &typeid(int(double)),  &typeid(int A::*), &typeid(const char*),
  };
  constexpr int N = sizeof(t) / sizeof(t[0]);
  for (int i = 0; i < N; ++i) {
    CHECK(*t[i] == *t[i]);
    CHECK(!t[i]->before(*t[i]));
    CHECK(t[i]->name() != nullptr);
    for (int j = 0; j < N; ++j) {
      if (i == j) continue;
      CHECK(!(*t[i] == *t[j]));
      CHECK(*t[i] != *t[j]);
      CHECK(!(t[i]->before(*t[j]) && t[j]->before(*t[i])));       // asymmetric
      CHECK(t[i]->before(*t[j]) || t[j]->before(*t[i]));          // distinct types are ordered
      for (int k = 0; k < N; ++k)
        if (t[i]->before(*t[j]) && t[j]->before(*t[k])) CHECK(t[i]->before(*t[k]));
    }
  }

  // equal types obtained by different means
  const std::type_info& d1 = typeid(ar);
  const std::type_info& d2 = typeid(B);
  const std::type_info& d3 = typeid(const B&);
  CHECK(d1 == d2 && d2 == d3);
  CHECK(!d1.before(d2) && !d2.before(d1));
  CHECK(d1.hash_code() == d2.hash_code() && d2.hash_code() == d3.hash_code());
  for (int i = 0; i < N; ++i) {
    if (*t[i] == d1) continue;
    CHECK(t[i]->before(d1) == t[i]->before(d2));
    CHECK(d1.before(*t[i]) == d2.before(*t[i]));
  }
  CHECK(typeid(Tmpl<int>).hash_code() == typeid(const Tmpl<int>).hash_code());
  return 0;
}
