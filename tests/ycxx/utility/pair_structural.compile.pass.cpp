// [pairs.pair]/4: "pair<T, U> is a structural type ([temp.param]) if T and U are both
// structural types. Two values p1 and p2 of type pair<T, U> are template-argument-equivalent
// ([temp.type]) if and only if p1.first and p2.first are template-argument-equivalent and
// p1.second and p2.second are template-argument-equivalent." /2: the defaulted copy/move
// constructors are constexpr when the element-wise initializations are. Class template
// argument deduction: "template<class T1, class T2> pair(T1, T2) -> pair<T1, T2>;" (by-value
// parameters: arrays and functions decay, reference_wrapper is kept as is).
// COUNTERPART: libstdcxx:20_util/pair/requirements/dr801.cc
#include <utility>
#include <functional>
#include <type_traits>

template <std::pair<int, char> P>
struct Holder {
  static constexpr int first = P.first;
};
static_assert(Holder<std::pair<int, char>(3, 'a')>::first == 3);
static_assert(std::is_same_v<Holder<std::pair<int, char>(1, 'x')>, Holder<std::pair<int, char>(1, 'x')>>);
static_assert(!std::is_same_v<Holder<std::pair<int, char>(1, 'x')>, Holder<std::pair<int, char>(1, 'y')>>);
template <auto V>
struct AutoHolder {};
static_assert(std::is_same_v<AutoHolder<std::pair(1, 2L)>, AutoHolder<std::pair<int, long>(1, 2)>>);

constexpr std::pair<int, double> src(1, 2.0);
constexpr std::pair<int, double> copied = src;
static_assert(copied.second == 2.0);
constexpr std::pair<int, double> moved = std::pair<int, double>(3, 4.0);
static_assert(moved.first == 3);

int fn(int);
void deduction() {
  int i = 0;
  std::pair a(1, 2.0);
  static_assert(std::is_same_v<decltype(a), std::pair<int, double>>);
  std::pair b("lit", fn);
  static_assert(std::is_same_v<decltype(b), std::pair<const char*, int (*)(int)>>);
  std::pair c(std::ref(i), i);
  static_assert(std::is_same_v<decltype(c), std::pair<std::reference_wrapper<int>, int>>);
  const int ci = 0;
  std::pair d(ci, ci);
  static_assert(std::is_same_v<decltype(d), std::pair<int, int>>);
  std::pair e = a;  // copy deduction
  static_assert(std::is_same_v<decltype(e), std::pair<int, double>>);
}
