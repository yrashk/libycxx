// [func.wrap.func.general]: template<class R, class... ArgTypes>
// function(R(*)(ArgTypes...)) -> function<R(ArgTypes...)>;
// [func.wrap.func.con]/16-17: template<class F> function(F) -> function<R(A...)> when
// decltype(&F::operator()) is R(G::*)(A...) cv &opt noexceptopt, R(*)(G, A...) noexceptopt
// (explicit object parameter) or, for a static operator(), R(*)(A...) noexceptopt.
#include <functional>
#include <type_traits>

int f0();
long f2(int, char) noexcept;
struct C1 {
  double operator()(int) const;
};
struct C2 {
  void operator()(int, int) & noexcept;
};
struct C3 {
  static short operator()(char);
};
struct C4 {
  int operator()(this const C4&, long);
};
struct C5 {
  int operator()(int) &&;
};

void test() {
  std::function a = f0;
  static_assert(std::is_same_v<decltype(a), std::function<int()>>);
  std::function b = &f2;
  static_assert(std::is_same_v<decltype(b), std::function<long(int, char)>>);
  std::function c = C1{};
  static_assert(std::is_same_v<decltype(c), std::function<double(int)>>);
  std::function d = [](int x) mutable { return x; };
  static_assert(std::is_same_v<decltype(d), std::function<int(int)>>);
  std::function e = [](float) noexcept {};
  static_assert(std::is_same_v<decltype(e), std::function<void(float)>>);
  std::function g{C3{}};
  static_assert(std::is_same_v<decltype(g), std::function<short(char)>>);
  std::function h{C4{}};
  static_assert(std::is_same_v<decltype(h), std::function<int(long)>>);
  int i = 5;
  std::function k = [&](double) { return i; };  // the draft's example
  static_assert(std::is_same_v<decltype(k), std::function<int(double)>>);
  std::function copy = k;  // copy deduction candidate
  static_assert(std::is_same_v<decltype(copy), std::function<int(double)>>);
}
// C2's operator() is lvalue-ref-qualified; the guide still applies (cv &opt).
using D2 = decltype(std::function{std::declval<C2>()});
static_assert(std::is_same_v<D2, std::function<void(int, int)>>);
// C5's operator() is &&-qualified, which is not of the form "cv &opt": no guide applies.
template <class T>
concept deducible = requires(T t) { std::function{t}; };
static_assert(deducible<C1>);
static_assert(!deducible<C5>);
