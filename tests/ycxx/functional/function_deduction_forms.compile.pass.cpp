// [func.wrap.func.con]/16: template<class F> function(F) -> function<see below>; "Constraints:
// &F::operator() is well-formed when treated as an unevaluated operand and either
// F::operator() is a non-static member function and decltype(&F::operator()) is either of the
// form R(G::*)(A...) cv &opt noexceptopt or of the form R(*)(G, A...) noexceptopt for a type
// G, or F::operator() is a static member function and decltype(&F::operator()) is of the form
// R(*)(A...) noexceptopt." /17: "The deduced type is function<R(A...)>."
// Covers the forms not exercised by function_deduction.compile.pass.cpp: volatile and
// const volatile, const& noexcept, explicit object parameters taken by value / noexcept, static noexcept, and the cases where &F::operator() is ill-formed
// (overloaded, template) or has a different form (C variadic, &&-qualified).
// (An explicit object parameter of type F&& would match the guide's R(*)(G, A...) form, but
// the resulting initialization is ill-formed by /9.2 -- FD& cannot bind to it -- so it is not
// observable through CTAD and is not tested.)
#include <functional>
#include <type_traits>
#include <utility>

struct V {
  int operator()(int) volatile;
};
struct CV {
  long operator()(char, char) const volatile;
};
struct CVRef {
  void operator()() const volatile&;
};
struct CRefNoexcept {
  double operator()(float) const& noexcept;
};
struct ByValueSelf {
  int operator()(this ByValueSelf, short);
};
struct NoexceptSelf {
  unsigned operator()(this const NoexceptSelf&) noexcept;
};
struct StaticNoexcept {
  static int* operator()(int*) noexcept;
};
struct ReturnsRef {
  int& operator()(int&) const;
};
struct Overloaded {
  int operator()(int) const;
  int operator()(long) const;
};
struct Template {
  template <class T>
  int operator()(T) const;
};
struct CVariadic {
  int operator()(int, ...) const;
};
struct RvalueQualified {
  int operator()() const&&;
};
struct NoCall {};

template <class F>
using deduced = decltype(std::function{std::declval<F>()});
template <class F>
concept deducible = requires(F f) { std::function{f}; };

static_assert(std::is_same_v<deduced<V>, std::function<int(int)>>);
static_assert(std::is_same_v<deduced<CV>, std::function<long(char, char)>>);
static_assert(std::is_same_v<deduced<CVRef>, std::function<void()>>);
static_assert(std::is_same_v<deduced<CRefNoexcept>, std::function<double(float)>>);
static_assert(std::is_same_v<deduced<ByValueSelf>, std::function<int(short)>>);
static_assert(std::is_same_v<deduced<NoexceptSelf>, std::function<unsigned()>>);
static_assert(std::is_same_v<deduced<StaticNoexcept>, std::function<int*(int*)>>);
static_assert(std::is_same_v<deduced<ReturnsRef>, std::function<int&(int&)>>);

static_assert(!deducible<Overloaded>);
static_assert(!deducible<Template>);
static_assert(!deducible<CVariadic>);
static_assert(!deducible<RvalueQualified>);
static_assert(!deducible<NoCall>);
static_assert(!deducible<int>);

// lambdas: mutable, noexcept, static, and explicit object parameter forms
void lambdas() {
  auto m = [n = 0](int) mutable { return ++n; };
  static_assert(std::is_same_v<decltype(std::function{m}), std::function<int(int)>>);
  auto s = [](double d) static noexcept { return d; };
  static_assert(std::is_same_v<decltype(std::function{s}), std::function<double(double)>>);
  auto e = [](this auto&&, int) { return 1L; };  // a template: &F::operator() is ill-formed
  static_assert(!deducible<decltype(e)>);
  auto g = [](auto x) { return x; };
  static_assert(!deducible<decltype(g)>);
}
