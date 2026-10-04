// [type.index]: operator< is target->before(*rhs.target); operator<=> returns
// strong_ordering::equal if *target == *rhs.target, less if target->before(*rhs.target),
// otherwise greater. [type.info]/before: "true if *this precedes rhs in the implementation's
// collation order" -- a total order on types, so across many distinct types type_index gives a
// strict total order consistent between <, >, <=, >=, == and <=>, and sorting works.
#include <typeindex>
#include <compare>
#include "check.hpp"

struct A {};
struct B {};
enum E {};
union U {};

int main() {
  const std::type_index ts[] = {typeid(int),    typeid(long),  typeid(char),       typeid(double),
                                typeid(A),      typeid(B),     typeid(E),          typeid(U),
                                typeid(int*),   typeid(A*),    typeid(void),       typeid(int[3]),
                                typeid(void()), typeid(int A::*), typeid(const char*), typeid(float)};
  constexpr int n = sizeof(ts) / sizeof(ts[0]);
  for (int i = 0; i < n; ++i) {
    CHECK(ts[i] == ts[i] && !(ts[i] < ts[i]) && (ts[i] <=> ts[i]) == 0);
    for (int j = 0; j < n; ++j) {
      if (i == j) continue;
      CHECK(ts[i] != ts[j]);
      CHECK((ts[i] < ts[j]) != (ts[j] < ts[i]));  // total: exactly one precedes
      CHECK((ts[i] < ts[j]) == (ts[j] > ts[i]));
      CHECK((ts[i] <= ts[j]) == (ts[i] < ts[j]));
      CHECK((ts[i] >= ts[j]) == (ts[i] > ts[j]));
      CHECK(((ts[i] <=> ts[j]) < 0) == (ts[i] < ts[j]));
      for (int k = 0; k < n; ++k)  // transitivity
        if (ts[i] < ts[j] && ts[j] < ts[k]) CHECK(ts[i] < ts[k]);
    }
  }
  // insertion sort with type_index::operator<
  std::type_index s[n] = {ts[0], ts[1], ts[2], ts[3], ts[4], ts[5], ts[6], ts[7],
                          ts[8], ts[9], ts[10], ts[11], ts[12], ts[13], ts[14], ts[15]};
  for (int i = 1; i < n; ++i)
    for (int j = i; j > 0 && s[j] < s[j - 1]; --j) {
      std::type_index t = s[j];
      s[j] = s[j - 1];
      s[j - 1] = t;
    }
  for (int i = 1; i < n; ++i) CHECK(s[i - 1] < s[i]);
  return 0;
}
