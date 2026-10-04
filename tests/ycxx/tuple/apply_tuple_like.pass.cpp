// apply, make_from_tuple and tuple_cat with every kind of tuple-like argument other than tuple:
// [tuple.like]/1: tuple-like means a specialization of array, complex, pair, tuple or
// ranges::subrange. [tuple.apply]/1: apply(f, t) is INVOKE(std::forward<F>(f),
// get<I>(std::forward<Tuple>(t))...) for I in [0, tuple_size_v<remove_reference_t<Tuple>>)
// (so members are passed with the value category of t, and a pointer to member works through
// INVOKE); /3: make_from_tuple<T>(t) is T(get<I>(std::forward<Tuple>(t))...).
// [tuple.creation]: tuple_cat(tpls...) for tuple-like Tuples gives tuple<CTypes...> with the
// elements' types ([tuple.creation]/10-12). The tuple protocol of the other types:
// [pair.astuple], [array.tuple], [complex.tuple] (tuple_size 2, element T, get<0> real part,
// get<1> imaginary part), [range.subrange.access] (get<0> begin, get<1> end, from a const&
// or an rvalue).
#include <tuple>
#include <array>
#include <complex>
#include <ranges>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Pt {
  int x;
  int y;
  constexpr Pt(int a, int b) : x(a), y(b) {}
  constexpr int sum() const { return x + y; }
};

struct Cat {
  constexpr int operator()(int&, int&) const { return 1; }        // lvalues
  constexpr int operator()(const int&, const int&) const { return 2; }
  constexpr int operator()(int&&, int&&) const { return 3; }      // rvalues
};

constexpr bool test() {
  // pair
  {
    std::pair<int, int> p(3, 4);
    if (std::apply([](int a, int b) { return a * 10 + b; }, p) != 34) return false;
    if (std::apply(Cat{}, p) != 1) return false;
    if (std::apply(Cat{}, std::as_const(p)) != 2) return false;
    if (std::apply(Cat{}, std::move(p)) != 3) return false;
    Pt q = std::make_from_tuple<Pt>(std::pair<int, int>(5, 6));
    if (q.x != 5 || q.y != 6) return false;
    // INVOKE with a pointer to member function and the object as the first element
    Pt obj(1, 2);
    if (std::apply(&Pt::sum, std::tuple<Pt&>(obj)) != 3) return false;
    if (std::apply(&Pt::x, std::array<Pt*, 1>{&obj}) != 1) return false;  // pointer to data member
  }
  // array
  {
    std::array<int, 2> a{7, 8};
    if (std::apply(Cat{}, a) != 1 || std::apply(Cat{}, std::move(a)) != 3) return false;
    Pt q = std::make_from_tuple<Pt>(a);
    if (q.sum() != 15) return false;
  }
  // complex: get<0> is the real part, get<1> the imaginary part
  {
    std::complex<double> c(1.5, -2.5);
    if (std::apply([](double re, double im) { return re - im; }, c) != 4.0) return false;
    std::apply([](double& re, double& im) { re = 10; im = 20; }, c);
    if (c.real() != 10 || c.imag() != 20) return false;
    static_assert(std::is_same_v<decltype(std::apply([](auto&& r, auto&&) -> decltype(auto) { return std::forward<decltype(r)>(r); },
                                                     std::declval<std::complex<float>&&>())),
                                 float&&>);
    auto t = std::make_from_tuple<std::pair<double, double>>(std::complex<double>(3, 4));
    if (t.first != 3 || t.second != 4) return false;
  }
  // subrange: (begin, end)
  {
    int arr[] = {1, 2, 3, 4};
    std::ranges::subrange<int*> sr(arr + 1, arr + 4);
    if (std::apply([](int* b, int* e) { return e - b; }, sr) != 3) return false;
    if (std::apply([](int* b, int*) { return *b; }, std::as_const(sr)) != 2) return false;
    auto pr = std::make_from_tuple<std::pair<int*, int*>>(sr);
    if (pr.first != arr + 1 || pr.second != arr + 4) return false;
  }
  // tuple_cat of mixed tuple-like arguments
  {
    int arr[] = {9, 9};
    auto t = std::tuple_cat(std::pair<int, char>(1, 'a'), std::array<long, 2>{2, 3}, std::complex<float>(4, 5),
                            std::ranges::subrange<int*>(arr, arr + 2), std::tuple<>());
    static_assert(std::is_same_v<decltype(t), std::tuple<int, char, long, long, float, float, int*, int*>>);
    if (std::get<0>(t) != 1 || std::get<1>(t) != 'a' || std::get<3>(t) != 3) return false;
    if (std::get<4>(t) != 4 || std::get<5>(t) != 5 || std::get<6>(t) != arr || std::get<7>(t) != arr + 2) return false;
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
  return 0;
}
