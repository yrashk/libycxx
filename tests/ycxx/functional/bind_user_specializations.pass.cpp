// [func.bind.isplace]/2: "A program may specialize this template for a program-defined type T
// to have a base characteristic of integral_constant<int, N> with N > 0 to indicate that T
// should be treated as a placeholder type." [func.bind.isbind]/2: likewise true_type for a
// subexpression. [func.bind.bind]/7.2-7.3: such arguments are treated as bind expressions
// (called with all the call arguments) and placeholders respectively.
#include <functional>
#include <type_traits>
#include "check.hpp"

using namespace std::placeholders;

struct Second {};
struct SumAll {
  int operator()(int a, int b) const { return a + b; }
};
template <>
struct std::is_placeholder<Second> : std::integral_constant<int, 2> {};
template <>
struct std::is_bind_expression<SumAll> : std::true_type {};

int sub(int a, int b) { return a - b; }

int main() {
  CHECK(std::bind(sub, Second{}, _1)(3, 10) == 7);
  // SumAll is evaluated with the call arguments (3, 10) and its result is passed
  CHECK(std::bind(sub, SumAll{}, _2)(3, 10) == 3);
  return 0;
}
