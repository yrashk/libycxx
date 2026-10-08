// Structured bindings look up std::tuple_size / std::tuple_element (a program's specializations
// and the library's).
#include <utility>
#include <tuple>
#include <cstdio>
struct P { int a, b; template <int I> int get() const { return I ? b : a; } };
template <> struct std::tuple_size<P> : std::integral_constant<std::size_t, 2> {};
template <std::size_t I> struct std::tuple_element<I, P> { using type = int; };
int main() {
  auto [x, y] = P{1, 2};
  auto [u, v] = std::pair{3, 4};
  auto [w, z] = std::tuple{5, 6};
  int s = x + y + u + v + w + z;
  std::puts(s == 21 ? "ok" : "FAIL");
  return s == 21 ? 0 : 1;
}
